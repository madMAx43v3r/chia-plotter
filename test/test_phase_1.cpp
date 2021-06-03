/*
 * test_phase_1.cpp
 *
 *  Created on: May 25, 2021
 *      Author: mad
 */

#include <chia/phase1.hpp>
#include <chia/DiskSort.hpp>

#include <iostream>

using namespace phase1;


int main(int argc, char** argv)
{
	const int num_threads = argc > 1 ? atoi(argv[1]) : 4;
	const int log_num_buckets = argc > 2 ? atoi(argv[2]) : 7;
	
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
	
	DiskSort1 sort_1(32 + kExtraBits, log_num_buckets, "test.p1.t1");
	compute_f1(id, num_threads, &sort_1);
	
	DiskSort2 sort_2(32 + kExtraBits, log_num_buckets, "test.p1.t2");
	compute_table<entry_1, entry_2, tmp_entry_1>(
			2, num_threads, &sort_1, &sort_2, tmp_files[0]);
	
	DiskSort3 sort_3(32 + kExtraBits, log_num_buckets, "test.p1.t3");
	compute_table<entry_2, entry_3, tmp_entry_x>(
			3, num_threads, &sort_2, &sort_3, tmp_files[1]);
	
	DiskSort4 sort_4(32 + kExtraBits, log_num_buckets, "test.p1.t4");
	compute_table<entry_3, entry_4, tmp_entry_x>(
			4, num_threads, &sort_3, &sort_4, tmp_files[2]);
	
	DiskSort5 sort_5(32 + kExtraBits, log_num_buckets, "test.p1.t5");
	compute_table<entry_4, entry_5, tmp_entry_x>(
			5, num_threads, &sort_4, &sort_5, tmp_files[3]);
	
	DiskSort6 sort_6(32 + kExtraBits, log_num_buckets, "test.p1.t6");
	compute_table<entry_5, entry_6, tmp_entry_x>(
			6, num_threads, &sort_5, &sort_6, tmp_files[4]);
	
	compute_table<entry_6, entry_7, tmp_entry_x, DiskSort6, DiskSort7>(
			7, num_threads, &sort_6, nullptr, tmp_files[5], tmp_files[6]);
	
	for(auto file : tmp_files) {
		fclose(file);
	}
	
	std::cout << "Phase 1 took " << (get_wall_time_micros() - total_begin) / 1e6 << " sec" << std::endl;
}


