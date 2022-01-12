/*
 * test_phase_4.cpp
 *
 *  Created on: Jun 3, 2021
 *      Author: mad
 */

#include <chia/phase4.hpp>

#include <iostream>

using namespace phase4;


int main(int argc, char** argv)
{
	const int num_threads = argc > 1 ? atoi(argv[1]) : 4;
	const int log_num_buckets = argc > 2 ? atoi(argv[2]) : 7;
	
	const int k = 32;
	const auto total_begin = get_wall_time_micros();
	
	FILE* plot_file = fopen("test.plot.tmp", "rb+");
	if(!plot_file) {
		throw std::runtime_error("fopen() failed");
	}
	
	int header_size = 0;
	uint64_t final_pointer_7 = 0;
	uint64_t num_written_final_7 = 0;
	{
		std::ifstream in("test.p3.header_size");
		in >> header_size;
	}
	{
		std::ifstream in("test.p3.final_pointer_7");
		in >> final_pointer_7;
	}
	{
		std::ifstream in("test.p3.num_written_final_7");
		in >> num_written_final_7;
	}
	std::cout << "[P4] header_size = " << header_size << std::endl;
	std::cout << "[P4] final_pointer_7 = " << final_pointer_7 << std::endl;
	std::cout << "[P4] num_written_final_7 = " << num_written_final_7 << std::endl;
	
	phase3::DiskSortNP L_sort_7(32, log_num_buckets, "test.p3s2.t7", true);
	
	const uint64_t total_plot_size =
			compute(plot_file, k, header_size, &L_sort_7, num_threads, final_pointer_7, num_written_final_7);
	
	fclose(plot_file);
	
	std::cout << "Phase 4 took " << (get_wall_time_micros() - total_begin) / 1e6 << " sec"
			", final plot size is " << total_plot_size << " bytes" << std::endl;
	return 0;
}


