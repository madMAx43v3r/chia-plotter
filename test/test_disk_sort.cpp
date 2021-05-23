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
	
	std::cout << "sizeof(phase1::entry_1) = " << sizeof(phase1::entry_1) << std::endl;
	
	const size_t test_bits = argc > 1 ? atoi(argv[1]) : 24;
	const size_t test_size = size_t(1) << test_bits;
	const size_t log_num_buckets = argc > 2 ? atoi(argv[2]) : 7;
	const size_t num_buckets = size_t(1) << log_num_buckets;
	{
		DiskSort<phase1::entry_1, std::less<uint64_t>, phase1::get_f<phase1::entry_1>>
			sort(test_bits, log_num_buckets, "test");
		
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
		const auto sort_begin = get_wall_time_micros();
		for(size_t i = 0; i < num_buckets; ++i) {
			const auto sorted = sort.read_bucket(i, 15113);
			for(const auto& block : sorted) {
				fwrite(block.data(), sizeof(phase1::entry_1), block.size(), out);
			}
		}
		std::cout << "sort() took " << (get_wall_time_micros() - sort_begin) / 1000.f << " ms" << std::endl;
		fclose(out);
	}
	
	return 0;
}


