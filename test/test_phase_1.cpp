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
	
	const int k = 32;
	const auto total_begin = get_wall_time_micros();
	
	initialize();
	
	DiskSort1 sort_1(k + kExtraBits, log_num_buckets, "test.p1.t1");
	compute_f1(id, k, num_threads, &sort_1);
	
	DiskTable<tmp_entry_1> tmp_1("test.p1.table1.tmp");
	DiskSort2 sort_2(k + kExtraBits, log_num_buckets, "test.p1.t2");
	compute_table<entry_1, entry_2, tmp_entry_1>(
			2, k, num_threads, &sort_1, &sort_2, &tmp_1);
	
	DiskTable<tmp_entry_x> tmp_2("test.p1.table2.tmp");
	DiskSort3 sort_3(k + kExtraBits, log_num_buckets, "test.p1.t3");
	compute_table<entry_2, entry_3, tmp_entry_x>(
			3, k, num_threads, &sort_2, &sort_3, &tmp_2);
	
	DiskTable<tmp_entry_x> tmp_3("test.p1.table3.tmp");
	DiskSort4 sort_4(k + kExtraBits, log_num_buckets, "test.p1.t4");
	compute_table<entry_3, entry_4, tmp_entry_x>(
			4, k, num_threads, &sort_3, &sort_4, &tmp_3);
	
	DiskTable<tmp_entry_x> tmp_4("test.p1.table4.tmp");
	DiskSort5 sort_5(k + kExtraBits, log_num_buckets, "test.p1.t5");
	compute_table<entry_4, entry_5, tmp_entry_x>(
			5, k, num_threads, &sort_4, &sort_5, &tmp_4);
	
	DiskTable<tmp_entry_x> tmp_5("test.p1.table5.tmp");
	DiskSort6 sort_6(k + kExtraBits, log_num_buckets, "test.p1.t6");
	compute_table<entry_5, entry_6, tmp_entry_x>(
			6, k, num_threads, &sort_5, &sort_6, &tmp_5);
	
	DiskTable<tmp_entry_x> tmp_6("test.p1.table6.tmp");
	DiskTable<entry_7> tmp_7("test.p1.table7.tmp");
	compute_table<entry_6, entry_7, tmp_entry_x, DiskSort6, DiskSort7>(
			7, k, num_threads, &sort_6, nullptr, &tmp_6, &tmp_7);
	
	std::cout << "Phase 1 took " << (get_wall_time_micros() - total_begin) / 1e6 << " sec" << std::endl;
	return 0;
}


