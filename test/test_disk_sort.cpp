/*
 * test_disk_sort.cpp
 *
 *  Created on: May 23, 2021
 *      Author: mad
 */

#include <chia/phase1.h>
#include <chia/DiskSort.hpp>

#include <random>
#include <iostream>


int main(int argc, char** argv)
{
	std::mt19937_64 generator;
	generator.seed(0);
	
	const size_t test_bits = argc > 1 ? atoi(argv[1]) : 24;
	const size_t test_size = size_t(1) << test_bits;
	const size_t log_num_buckets = argc > 2 ? atoi(argv[2]) : 7;
//	const size_t num_buckets = size_t(1) << log_num_buckets;
	const size_t num_threads = 4;
	
	if(true) {
		std::cout << "sizeof(phase1::entry_1) = " << sizeof(phase1::entry_1) << std::endl;
		
		typedef DiskSort<phase1::entry_1, phase1::get_y<phase1::entry_1>> DiskSort1;
		
		DiskSort1 sort(test_bits, log_num_buckets, "test");
		
		const auto add_begin = get_wall_time_micros();
		for(size_t i = 0; i < test_size; ++i) {
			phase1::entry_1 entry = {};
			entry.y = generator() % test_size;
			entry.x = i;
			sort.add(entry);
		}
		sort.finish();
		std::cout << "add() took " << (get_wall_time_micros() - add_begin) / 1000. << " ms" << std::endl;
		
		FILE* out = fopen("sorted.out", "wb");
		
		Thread<std::pair<std::vector<phase1::entry_1>, size_t>> thread(
			[out](std::pair<std::vector<phase1::entry_1>, size_t>& input) {
				for(const auto& entry : input.first) {
					write_entry(out, entry);
				}
			}, "test_output");
		
		const auto sort_begin = get_wall_time_micros();
		sort.read(&thread, num_threads);
		fclose(out);
		std::cout << "sort() took " << (get_wall_time_micros() - sort_begin) / 1000. << " ms" << std::endl;
	}
	
	if(false) {
		std::cout << "sizeof(phase1::entry_4) = " << sizeof(phase1::entry_4) << std::endl;
		
		typedef DiskSort<phase1::entry_4, phase1::get_y<phase1::entry_4>> DiskSort4;
		
		DiskSort4 sort(test_bits, log_num_buckets, "test");
		
		const auto add_begin = get_wall_time_micros();
		for(size_t i = 0; i < test_size; ++i) {
			phase1::entry_4 entry = {};
			entry.y = generator() % test_size;
			entry.pos = i;
			sort.add(entry);
		}
		sort.finish();
		std::cout << "add() took " << (get_wall_time_micros() - add_begin) / 1000. << " ms" << std::endl;
		
		uint64_t f_max = 0;
		FILE* out = fopen("sorted.out", "wb");
		
		Thread<std::pair<std::vector<phase1::entry_4>, size_t>> thread(
			[out, &f_max](std::pair<std::vector<phase1::entry_4>, size_t>& input) {
				for(const auto& entry : input.first) {
					write_entry(out, entry);
					if(entry.y < f_max) {
						throw std::logic_error("entry.f < f_max");
					}
					f_max = entry.y;
				}
			}, "test_output");
		
		const auto sort_begin = get_wall_time_micros();
		sort.read(&thread, num_threads);
		fclose(out);
		std::cout << "sort() took " << (get_wall_time_micros() - sort_begin) / 1000. << " ms" << std::endl;
	}
	
	return 0;
}


