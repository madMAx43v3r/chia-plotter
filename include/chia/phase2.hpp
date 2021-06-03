/*
 * phase2.hpp
 *
 *  Created on: May 29, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_PHASE2_HPP_
#define INCLUDE_CHIA_PHASE2_HPP_

#include <chia/phase2.h>
#include <chia/DiskTable.h>
#include <chia/ThreadPool.h>

#include <chia/bitfield_index.hpp>


namespace phase2 {

template<typename T, typename S, typename DS>
void compute_table(	int R_index, int num_threads,
					DS* R_sort, DiskTable<S>* R_file,
					const table_t& R_table,
					bitfield* L_used,
					const bitfield* R_used)
{
	DiskTable<T> R_input(R_table.file_name, R_table.num_entries);
	{
		const auto begin = get_wall_time_micros();
		
		ThreadPool<std::pair<std::vector<T>, size_t>, size_t> pool(
			[L_used, R_used](std::pair<std::vector<T>, size_t>& input, size_t&, size_t&) {
				uint64_t offset = 0;
				for(const auto& entry : input.first) {
					if(R_used && !R_used->get(input.second + (offset++))) {
						continue;	// drop it
					}
					L_used->set(entry.pos);
					L_used->set(uint64_t(entry.pos) + entry.off);
				}
			}, nullptr, num_threads, "phase2/mark");
		
		L_used->clear();
		R_input.read(&pool);
		pool.close();
		
		std::cout << "[P2] Table " << R_index << " scan took "
				<< (get_wall_time_micros() - begin) / 1e6 << " sec" << std::endl;
	}
	const auto begin = get_wall_time_micros();
	
	uint64_t num_written = 0;
	const bitfield_index index(*L_used);
	
	typedef typename DS::WriteCache WriteCache;
	
	Thread<std::vector<S>> R_write(
		[R_file](std::vector<S>& input) {
			for(auto& entry : input) {
				R_file->write(entry);
			}
		}, "phase2/write");
	
	ThreadPool<std::vector<S>, size_t, std::shared_ptr<WriteCache>> R_add(
		[R_sort](std::vector<S>& input, size_t&, std::shared_ptr<WriteCache>& cache) {
			if(!cache) {
				cache = R_sort->add_cache();
			}
			for(auto& entry : input) {
				cache->add(entry);
			}
		}, nullptr, std::max(num_threads / 2, 1), "phase2/add");
	
	Processor<std::vector<S>>* R_out = &R_add;
	if(R_file) {
		R_out = &R_write;
	}
	
	Thread<std::vector<S>> R_count(
		[R_out, &num_written](std::vector<S>& input) {
			for(auto& entry : input) {
				set_sort_key<S>{}(entry, num_written++);
			}
			R_out->take(input);
		}, "phase2/count");
	
	ThreadPool<std::pair<std::vector<T>, size_t>, std::vector<S>> map_pool(
		[&index, R_used](std::pair<std::vector<T>, size_t>& input, std::vector<S>& out, size_t&) {
			out.reserve(input.first.size());
			uint64_t offset = 0;
			for(const auto& entry : input.first) {
				if(R_used && !R_used->get(input.second + (offset++))) {
					continue;	// drop it
				}
				S tmp;
				tmp.assign(entry);
				const auto pos_off = index.lookup(entry.pos, entry.off);
				tmp.pos = pos_off.first;
				tmp.off = pos_off.second;
				out.push_back(tmp);
			}
		}, &R_count, num_threads, "phase2/remap");
	
	R_input.read(&map_pool);
	
	map_pool.close();
	R_count.close();
	R_write.close();
	R_add.close();
	
	if(R_sort) {
		R_sort->finish();
	}
	if(R_file) {
		R_file->flush();
	}
	std::cout << "[P2] Table " << R_index << " rewrite took "
				<< (get_wall_time_micros() - begin) / 1e6 << " sec"
				<< ", dropped " << R_table.num_entries - num_written << " entries"
				<< " (" << 100 * (1 - double(num_written) / R_table.num_entries) << " %)" << std::endl;
}


} // phase2

#endif /* INCLUDE_CHIA_PHASE2_HPP_ */
