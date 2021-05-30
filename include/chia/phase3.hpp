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

template<	typename T, typename S,
			typename DS_L, typename DS_R, typename DS_L2, typename DS_R2>
uint64_t compute_table(	int L_index, int num_threads,
						DS_L* L_sort, DS_R* R_sort, DS_L2* L_sort_2, DS_R2* R_sort_2,
						DiskTable<T>* L_table, bitfield const* L_used)
{
	const auto begin_1 = get_wall_time_micros();
	
	std::mutex mutex;
	std::condition_variable signal;
	static constexpr size_t M = 65536;
	
	bool L_is_end = false;
	bool R_is_waiting = false;
	uint64_t L_offset = 0;
	uint64_t R_num_write = 0;
	std::vector<uint32_t> L_buffer;
	
	Thread<std::pair<std::vector<T>, size_t>> L_read_1(
		[&mutex, &signal, L_used, &L_buffer, &R_is_waiting](std::pair<std::vector<T>, size_t>& input) {
			std::unique_lock<std::mutex> lock(mutex);
			while(L_buffer.size() > M && !R_is_waiting) {
				signal.wait(lock);
			}
			size_t offset = 0;
			for(const auto& entry : input.first) {
				if(!L_used->get(input.second + (offset++))) {
					continue;	// drop it
				}
				L_buffer.push_back(get_new_pos<T>{}(entry));
			}
//			std::cout << "L_read_1: L_buffer size = " << L_buffer.size() << std::endl;
			lock.unlock();
			signal.notify_all();
		}, "phase3/buffer/L");
	
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
		}, "phase3/buffer/L");
	
	Thread<std::vector<entry_lp>> L_add_2(
		[L_sort_2, &R_num_write](std::vector<entry_lp>& input) {
			for(const auto& entry : input) {
				L_sort_2->add(entry);
			}
			R_num_write += input.size();
		}, "phase3/add/R");
	
	Thread<std::vector<S>> R_read(
		[&mutex, &signal, &L_offset, &L_buffer, &L_is_end, &R_is_waiting, &L_add_2](std::vector<S>& input) {
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
//					std::cout << "R_read: waiting for offset " << pos[1] << std::endl;
					signal.notify_all();
					signal.wait(lock);
				}
				R_is_waiting = false;
				if(pos[1] >= L_buffer.size()) {
					continue;	// skip out of bounds
				}
				const uint128_t line_point = Encoding::SquareToLinePoint(
						L_buffer[pos[0]], L_buffer[pos[1]]);
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
			L_add_2.take(out);
		}, "phase3/lp_conv/R");
	
	std::thread R_sort_read(
		[R_sort, &R_read]() {
			R_sort->read(&R_read);
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
	R_read.close();
	L_add_2.close();
	L_sort_2->finish();
	
	std::cout << "[P3] Table " << L_index + 1 << " line point rewrite took "
				<< (get_wall_time_micros() - begin_1) / 1e6 << " sec"
				<< ", wrote " << R_num_write << " entries";
	
	// TODO
	
	return R_num_write;
}


} // phase3

#endif /* INCLUDE_CHIA_PHASE3_HPP_ */
