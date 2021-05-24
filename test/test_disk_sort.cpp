/*
 * test_disk_sort.cpp
 *
 *  Created on: May 23, 2021
 *      Author: mad
 */

#include <chia/entries.h>
#include <chia/DiskSort.hpp>

#include <random>
#include <chrono>
#include <iostream>

int64_t get_wall_time_micros() {
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}


int main(int argc, char** argv)
{
	std::mt19937_64 generator;
	generator.seed(0);
	
	const size_t test_bits = argc > 1 ? atoi(argv[1]) : 24;
	const size_t test_size = size_t(1) << test_bits;
	const size_t log_num_buckets = argc > 2 ? atoi(argv[2]) : 7;
	const size_t num_buckets = size_t(1) << log_num_buckets;
	
	if(false) {
		std::cout << "sizeof(phase1::entry_1) = " << sizeof(phase1::entry_1) << std::endl;
		
		typedef DiskSort<phase1::entry_1, phase1::get_f<phase1::entry_1>> DiskSort1;
		
		DiskSort1 sort(test_bits, log_num_buckets, "test");
		
		const auto add_begin = get_wall_time_micros();
		for(size_t i = 0; i < test_size; ++i) {
			phase1::entry_1 entry = {};
			entry.f = generator() % test_size;
			entry.x = i;
			sort.add(entry);
		}
		sort.finish();
		std::cout << "add() took " << (get_wall_time_micros() - add_begin) / 1000.f << " ms" << std::endl;
		
		FILE* out = fopen("sorted.out", "wb");
		
		Thread<DiskSort1::output_t> thread(
			[out](DiskSort1::output_t& input) {
				for(const auto& entry : input.block) {
					write_entry(out, entry);
				}
			}, "test_output");
		
		const auto sort_begin = get_wall_time_micros();
		sort.read(thread, 15113);
		fclose(out);
		std::cout << "sort() took " << (get_wall_time_micros() - sort_begin) / 1000.f << " ms" << std::endl;
	}
	
	if(true) {
		std::cout << "sizeof(phase1::entry_4) = " << sizeof(phase1::entry_4) << std::endl;
		
		typedef DiskSort<phase1::entry_4, phase1::get_f<phase1::entry_4>> DiskSort4;
		
		DiskSort4 sort(test_bits, log_num_buckets, "test");
		
		const auto add_begin = get_wall_time_micros();
		for(size_t i = 0; i < test_size; ++i) {
			phase1::entry_4 entry = {};
			entry.f = generator() % test_size;
			entry.pos = i;
			sort.add(entry);
		}
		sort.finish();
		std::cout << "add() took " << (get_wall_time_micros() - add_begin) / 1000.f << " ms" << std::endl;
		
		FILE* out = fopen("sorted.out", "wb");
		
		Thread<DiskSort4::output_t> thread(
			[out](DiskSort4::output_t& input) {
				for(const auto& entry : input.block) {
					write_entry(out, entry);
				}
			}, "test_output");
		
		const auto sort_begin = get_wall_time_micros();
		sort.read(thread, 15113);
		fclose(out);
		std::cout << "sort() took " << (get_wall_time_micros() - sort_begin) / 1000.f << " ms" << std::endl;
	}
	
	return 0;
}


