/*
 * test_phase_1.cpp
 *
 *  Created on: May 25, 2021
 *      Author: mad
 */

#include <chia/entries.h>
#include <chia/phase1.h>
#include <chia/DiskSort.hpp>

#include <random>
#include <chrono>
#include <iostream>

int64_t get_wall_time_micros() {
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}


int main(int argc, char** argv)
{
	const size_t log_num_buckets = argc > 1 ? atoi(argv[1]) : 9;
	const size_t num_threads = 4;
	
	uint8_t id[32] = {};
	
	typedef DiskSort<phase1::entry_1, phase1::get_y<phase1::entry_1>> DiskSort1;
	
	DiskSort1 sort_1(32 + kExtraBits, log_num_buckets, num_threads, "test.p1.t1");
	{
		Thread<std::vector<phase1::entry_1>> thread(
			[&sort_1](std::vector<phase1::entry_1>& input) {
				for(const auto& entry : input) {
					sort_1.add(entry);
				}
			}, "DiskSort/add");
		phase1::compute_f1(id, num_threads, &thread);
	}
	sort_1.finish();
	
}


