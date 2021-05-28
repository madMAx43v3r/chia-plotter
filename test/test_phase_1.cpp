/*
 * test_phase_1.cpp
 *
 *  Created on: May 25, 2021
 *      Author: mad
 */

#include <chia/entries.h>
#include <chia/phase1.hpp>
#include <chia/DiskSort.hpp>

#include <iostream>

using namespace phase1;


int main(int argc, char** argv)
{
	const size_t num_threads = 4;
	const size_t num_threads_sort = 2;
	const size_t log_num_buckets = argc > 1 ? atoi(argv[1]) : 8;
	
	uint8_t id[32] = {};
	for(size_t i = 0; i < sizeof(id); ++i) {
		id[i] = i + 1;
	}
	
	const auto total_begin = get_wall_time_micros();
	
	initialize();
	
	std::array<FILE*, 7> tmp_files;
	for(size_t i = 0; i < tmp_files.size(); ++i) {
		tmp_files[i] = fopen(("test.p1.table" + std::to_string(i + 1) + ".tmp").c_str(), "wb");
	}
	
	typedef DiskSort<entry_1, get_y<entry_1>> DiskSort1;
	typedef DiskSort<entry_2, get_y<entry_2>> DiskSort2;
	typedef DiskSort<entry_3, get_y<entry_3>> DiskSort3;
	typedef DiskSort<entry_4, get_y<entry_4>> DiskSort4;
	typedef DiskSort<entry_5, get_y<entry_5>> DiskSort5;
	typedef DiskSort<entry_6, get_y<entry_6>> DiskSort6;
	typedef DiskSort<entry_7, get_y<entry_7>> DiskSort7;
	
	DiskSort1 sort_1(32 + kExtraBits, log_num_buckets, num_threads_sort, "test.p1.t1");
	{
		Thread<std::vector<entry_1>> output(
			[&sort_1](std::vector<entry_1>& input) {
				for(const auto& entry : input) {
					sort_1.add(entry);
//					std::cout << "x=" << entry.x << ", y=" << entry.y << std::endl;
				}
			}, "Disk/add");
		
		const auto begin = get_wall_time_micros();
		compute_f1(id, num_threads, &output);
		output.wait();
		sort_1.finish();
		
		std::cout << "Table 1 took " << (get_wall_time_micros() - begin) / 1e6 << " sec" << std::endl;
	}
	
	DiskSort2 sort_2(32 + kExtraBits, log_num_buckets, num_threads_sort, "test.p1.t2");
	compute_table<entry_1, entry_2, tmp_entry_1, tmp_entry_t>(
			2, num_threads, &sort_1, &sort_2, tmp_files[0]);
	sort_1.close();
	
	DiskSort3 sort_3(32 + kExtraBits, log_num_buckets, num_threads_sort, "test.p1.t3");
	compute_table<entry_2, entry_3, tmp_entry_t, tmp_entry_t>(
			3, num_threads, &sort_2, &sort_3, tmp_files[1]);
	sort_2.close();
	
	DiskSort4 sort_4(32 + kExtraBits, log_num_buckets, num_threads_sort, "test.p1.t4");
	compute_table<entry_3, entry_4, tmp_entry_t, tmp_entry_t>(
			4, num_threads, &sort_3, &sort_4, tmp_files[2]);
	sort_3.close();
	
	DiskSort5 sort_5(32 + kExtraBits, log_num_buckets, num_threads_sort, "test.p1.t5");
	compute_table<entry_4, entry_5, tmp_entry_t, tmp_entry_t>(
			5, num_threads, &sort_4, &sort_5, tmp_files[3]);
	sort_4.close();
	
	DiskSort6 sort_6(32 + kExtraBits, log_num_buckets, num_threads_sort, "test.p1.t6");
	compute_table<entry_5, entry_6, tmp_entry_t, tmp_entry_t>(
			6, num_threads, &sort_5, &sort_6, tmp_files[4]);
	sort_5.close();
	
	compute_table<entry_6, entry_7, tmp_entry_t, tmp_entry_t, DiskSort6, DiskSort7>(
			7, num_threads, &sort_6, nullptr, tmp_files[5], tmp_files[6]);
	sort_6.close();
	
	std::cout << "Phase 1 took " << (get_wall_time_micros() - total_begin) / 1e6 << " sec" << std::endl;
}


