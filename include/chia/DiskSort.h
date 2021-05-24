/*
 * DiskSort.h
 *
 *  Created on: May 23, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_DISKSORT_H_
#define INCLUDE_CHIA_DISKSORT_H_

#include <chia/ThreadPool.h>

#include <vector>
#include <string>
#include <cstdio>
#include <cstddef>
#include <memory>
#include <functional>


template<typename T, typename Key>
class DiskSort {
public:
	struct output_t {
		bool is_begin = false;		// start of a sort bucket
		bool is_end = false;		// end of a sort bucket
		std::vector<T> block;
	};
	
	DiskSort(int key_size, int log_num_buckets, int num_threads, std::string file_prefix);
	
	~DiskSort() {
		clear();
	}
	
	void read(Thread<output_t>* output, size_t M);
	
	void finish();
	
	void clear();
	
	void add(const T& entry);
	
	size_t num_buckets() const {
		return buckets.size();
	}
	
private:
	struct bucket_t {
		FILE* file = nullptr;
		std::string file_name;
		size_t offset = 0;
		size_t num_entries = 0;
		uint8_t buffer[262144];
		void flush();
	};
	
	void read_bucket(	const size_t index, const size_t M,
						Thread<std::vector<std::vector<T>>>* sort);
	
	void sort_bucket(	std::vector<std::vector<T>>& input,
						Thread<output_t>* output);
	
	void sort_block(	output_t& input, output_t& out);
	
private:
	int key_size = 0;
	int log_num_buckets = 0;
	int num_threads = 0;
	bool is_finished = false;
	std::vector<bucket_t> buckets;
	
};



#endif /* INCLUDE_CHIA_DISKSORT_H_ */
