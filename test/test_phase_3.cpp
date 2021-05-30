/*
 * test_phase_3.cpp
 *
 *  Created on: May 30, 2021
 *      Author: mad
 */

#include <chia/phase3.hpp>
#include <chia/DiskSort.hpp>
#include <chia/DiskTable.h>

#include <iostream>

using namespace phase3;


int main(int argc, char** argv)
{
	const int num_threads = argc > 1 ? atoi(argv[1]) : 4;
	const int log_num_buckets = argc > 2 ? atoi(argv[2]) : 8;
	
	const auto total_begin = get_wall_time_micros();
	
	phase1::table_t table_1;
	table_1.file_name = "test.p1.table1.tmp";
	table_1.num_entries = get_file_size(table_1.file_name.c_str()) / phase2::entry_1::disk_size;
	
	bitfield bitfield_1(table_1.num_entries);
	{
		FILE* file = fopen("test.p2.bitfield1.tmp", "rb");
		if(!file) {
			throw std::runtime_error("bitfield1 missing");
		}
		bitfield_1.read(file);
	}
	
	DiskTable<phase2::entry_1> L_table_1(table_1.file_name, table_1.num_entries);
	
	phase2::DiskSortT R_sort_po_2(32, log_num_buckets, num_threads, "test.p2.t2", true);
	DiskSortLP L_sort_lp_2(63, log_num_buckets, num_threads, "test.p3s1.t2");
	
	compute_table<	phase2::entry_1, phase2::entry_x,
					DiskSortNP, phase2::DiskSortT, DiskSortLP, DiskSortNP>(
			1, num_threads, nullptr, &R_sort_po_2, &L_sort_lp_2, nullptr, &L_table_1, &bitfield_1);
	
	R_sort_po_2.set_keep_files(true);
	
	// TODO
	
	std::cout << "Phase 3 took " << (get_wall_time_micros() - total_begin) / 1e6 << " sec" << std::endl;
}

