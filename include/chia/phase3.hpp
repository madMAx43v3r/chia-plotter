/*
 * phase3.hpp
 *
 *  Created on: May 30, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_PHASE3_HPP_
#define INCLUDE_CHIA_PHASE3_HPP_

#include <chia/chia.h>
#include <chia/phase3.h>
#include <chia/encoding.hpp>
#include <chia/DiskTable.h>

#include <list>


namespace phase3 {

template<typename T, typename S, typename DS_L, typename DS_R>
void compute_stage1(int L_index, int num_threads,
					DS_L* L_sort, DS_R* R_sort, DiskSortLP* R_sort_2,
					DiskTable<T>* L_table = nullptr, bitfield const* L_used = nullptr,
					DiskTable<S>* R_table = nullptr)
{
	const auto begin = get_wall_time_micros();
	
	std::mutex mutex;
	std::condition_variable signal;
	static constexpr size_t M = 65536;
	
	bool L_is_end = false;
	bool R_is_waiting = false;
	uint64_t L_offset = 0;
	uint64_t R_num_write = 0;
	std::vector<uint32_t> L_buffer;
	
	Thread<std::vector<entry_np>> L_read(
		[&mutex, &signal, &L_buffer, &R_is_waiting](std::vector<entry_np>& input) {
			std::unique_lock<std::mutex> lock(mutex);
			while(L_buffer.size() > M && !R_is_waiting) {
				signal.wait(lock);
			}
			for(const auto& entry : input) {
				L_buffer.push_back(entry.pos);
			}
			lock.unlock();
			signal.notify_all();
		}, "phase3/buffer");
	
	Thread<std::pair<std::vector<T>, size_t>> L_read_1(
		[L_used, &L_read](std::pair<std::vector<T>, size_t>& input) {
			std::vector<entry_np> out;
			out.reserve(input.first.size());
			size_t offset = 0;
			for(const auto& entry : input.first) {
				if(!L_used->get(input.second + (offset++))) {
					continue;	// drop it
				}
				entry_np tmp;
				tmp.pos = get_new_pos<T>{}(entry);
				out.push_back(tmp);
			}
			L_read.take(out);
		}, "phase3/filter_1");
	
	Thread<std::vector<entry_lp>> R_add_2(
		[R_sort_2, &R_num_write](std::vector<entry_lp>& input) {
			for(const auto& entry : input) {
				R_sort_2->add(entry);
			}
			R_num_write += input.size();
		}, "phase3/add");
	
	Thread<std::vector<S>> R_read(
		[&mutex, &signal, &L_offset, &L_buffer, &L_is_end, &R_is_waiting, &R_add_2](std::vector<S>& input) {
			std::vector<entry_lp> out;
			out.reserve(input.size());
			std::unique_lock<std::mutex> lock(mutex);
			for(const auto& entry : input) {
				uint64_t pos[2];
				pos[0] = entry.pos;
				pos[1] = uint64_t(entry.pos) + entry.off;
				if(pos[1] >= uint64_t(1) << 32) {
					continue;	// skip 32-bit overflow
				}
				if(pos[0] < L_offset) {
					continue;	// skip underflow (should never happen)
				}
				pos[0] -= L_offset;
				pos[1] -= L_offset;
				if(pos[1] >= (1 << 24)) {
					throw std::logic_error("buffer offset overflow");
				}
				while(L_buffer.size() <= pos[1] && !L_is_end) {
					R_is_waiting = true;
					signal.notify_all();
					signal.wait(lock);
				}
				R_is_waiting = false;
				if(pos[1] >= L_buffer.size()) {
					continue;	// skip out of bounds
				}
				const uint128_t line_point =
						Encoding::SquareToLinePoint(L_buffer[pos[0]], L_buffer[pos[1]]);
				
				entry_lp tmp;
				tmp.point = line_point;
				tmp.key = get_sort_key<S>{}(entry);
				out.push_back(tmp);
				
				if(pos[0] > M && L_buffer.size() > M) {
					// delete old buffer data
					L_offset += M;
					L_buffer.erase(L_buffer.begin(), L_buffer.begin() + M);
					lock.unlock();
					signal.notify_all();
					lock.lock();
				}
			}
			R_add_2.take(out);
		}, "phase3/lp_conv");
	
	Thread<std::pair<std::vector<S>, size_t>> R_read_7(
		[&R_read](std::pair<std::vector<S>, size_t>& input) {
			R_read.take(input.first);
		}, "phase3/buffer");
	
	std::thread R_sort_read(
		[R_sort, R_table, &R_read, &R_read_7]() {
			if(R_table) {
				R_table->read(&R_read_7);
				R_read_7.close();
			} else {
				R_sort->read(&R_read);
				R_read.close();
			}
		});
	
	if(L_table) {
		L_table->read(&L_read_1);
		L_read_1.close();
	} else {
		L_sort->read(&L_read);
		L_read.close();
	}
	{
		std::lock_guard<std::mutex> lock(mutex);
		L_is_end = true;
		signal.notify_all();
	}
	R_sort_read.join();
	R_add_2.close();
	R_sort_2->finish();
	
	std::cout << "[P3/1] Table " << L_index + 1 << " took "
				<< (get_wall_time_micros() - begin) / 1e6 << " sec"
				<< ", wrote " << R_num_write << " entries" << std::endl;
}

inline
void compute_stage2(int L_index, int num_threads,
					DiskSortLP* R_sort, DiskSortNP* L_sort)
{
	const auto begin = get_wall_time_micros();
	
	uint64_t R_num_read = 0;
	uint64_t L_num_write = 0;
	uint64_t last_point = 0;
	uint64_t check_point = 0;
	uint64_t park_index = 0;
	
	std::vector<uint8_t> park_deltas;
	std::vector<uint64_t> park_stubs;
	
	Thread<std::vector<entry_np>> L_add(
		[L_sort, &L_num_write](std::vector<entry_np>& input) {
			for(const auto& entry : input) {
				L_sort->add(entry);
			}
			L_num_write += input.size();
		}, "phase3/add");
	
	Thread<std::vector<entry_lp>> R_read(
		[&last_point, &check_point, &park_index, &park_deltas, &park_stubs, &R_num_read, &L_add]
		 (std::vector<entry_lp>& input) {
			std::vector<entry_np> out;
			out.reserve(input.size());
			for(const auto& entry : input) {
				const auto index = R_num_read++;
				if(index >= uint64_t(1) << 32) {
					continue;		// skip 32-bit position overflow
				}
				entry_np tmp;
				tmp.key = entry.key;
				tmp.pos = index;
				out.push_back(tmp);
				
				// Every EPP entries, writes a park
				if(index % kEntriesPerPark == 0 && index != 0) {
					// TODO
					park_deltas.clear();
					park_stubs.clear();
					check_point = entry.point;
				}
				const auto big_delta = entry.point - last_point;
				const auto stub = big_delta & ((1ull << (32 - kStubMinusBits)) - 1);
				const auto small_delta = big_delta >> (32 - kStubMinusBits);
				
				if(small_delta >= 256) {
					throw std::logic_error("small_delta >= 256");
				}
				if(index % kEntriesPerPark) {
					park_deltas.push_back(small_delta);
					park_stubs.push_back(stub);
				}
				last_point = entry.point;
			}
			L_add.take(out);
		}, "phase3/lp_delta");
	
	R_sort->read(&R_read);
	R_read.close();
	L_add.close();
	L_sort->finish();
	
	std::cout << "[P3/2] Table " << L_index + 1 << " took "
				<< (get_wall_time_micros() - begin) / 1e6 << " sec"
				<< ", wrote " << L_num_write << " entries" << std::endl;
}


} // phase3

#endif /* INCLUDE_CHIA_PHASE3_HPP_ */
