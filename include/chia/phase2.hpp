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


namespace phase2 {

template<typename T, typename S, typename DS>
void compute_table(	int R_index, int num_threads,
					DS* R_sort, FILE* R_out,
					const phase1::table_t& L_table,
					const phase1::table_t& R_table,
					bitfield* L_used,
					const bitfield* R_used)
{
	DiskTable<T> R_input(R_table.file_name, R_table.num_entries);
	{
		const auto begin = get_wall_time_micros();
		
		ThreadPool<std::vector<T>, size_t> pool(
			[L_used](std::vector<T>& input, size_t&, size_t&) {
				for(const auto& entry : input) {
					L_used->set(entry.pos);
					L_used->set(size_t(entry.pos) + entry.off);
				}
			}, nullptr, num_threads, "phase2/mark");
		
		L_used->clear();
		R_input.read(&pool);
		pool.wait();
		
		std::cout << "[P2] Table " << R_index << " scan took "
				<< (get_wall_time_micros() - begin) / 1e6 << " sec" << std::endl;
	}
	
	const auto num_used = L_used->count(0, L_table.num_entries);
	std::cout << num_used << " used entries ("
			<< (double(num_used) / L_table.num_entries) * 100 << " %)" << std::endl;
	
	// TODO
}


} // phase2

#endif /* INCLUDE_CHIA_PHASE2_HPP_ */
