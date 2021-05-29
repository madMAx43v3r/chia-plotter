/*
 * phase2.hpp
 *
 *  Created on: May 29, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_PHASE2_HPP_
#define INCLUDE_CHIA_PHASE2_HPP_

#include <chia/phase1.h>
#include <chia/phase2.h>
#include <chia/DiskTable.h>
#include <chia/ThreadPool.h>


namespace phase2 {

template<typename T, typename DS>
void compute_table(	int R_index, DS* R_sort,
					const phase1::table_t& L_table,
					const phase1::table_t& R_table,
					std::vector<bool>* L_used,
					const std::vector<bool>* R_used)
{
	DiskTable<T> R_input(R_table.file_name, R_table.num_entries);
	
	L_used->resize(L_table.num_entries, false);
	{
		const auto begin = get_wall_time_micros();
		
		Thread<std::vector<T>> mark_thread(
			[&L_used](std::vector<T>& input) {
				for(const auto& entry : input) {
					L_used[entry.pos] = true;
					L_used[size_t(entry.pos) + entry.off] = true;
				}
			}, "phase2/mark");
		
		R_input.read(&mark_thread);
		mark_thread.wait();
		
		std::cout << "[P2] Table " << R_index << " scan took "
				<< (get_wall_time_micros() - begin) / 1e6 << " sec" << std::endl;
	}
	
	// TODO
}


} // phase2

#endif /* INCLUDE_CHIA_PHASE2_HPP_ */
