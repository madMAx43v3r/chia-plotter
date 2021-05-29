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
	DiskSort(	int key_size, int log_num_buckets, int num_threads,
				std::string file_prefix, int num_threads_read = 2);
	
	~DiskSort() {
		close();
	}
	
	void read(Processor<std::vector<T>>* output);
	
	void finish();
	
	void close();
	
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
	
	void read_bucket(size_t& index, std::vector<std::vector<T>>& out);
	
	void sort_bucket(std::vector<std::vector<T>>& input, Processor<std::vector<T>>* sort);
	
	void sort_block(std::vector<T>& input, std::vector<T>& out);
	
private:
	const int key_size = 0;
	const int log_num_buckets = 0;
	const int bucket_key_shift = 0;
	const int num_threads = 0;
	const int num_threads_read = 0;
	
	bool is_finished = false;
	std::atomic<double> avg_block_size {};
	std::vector<bucket_t> buckets;
	
};



#endif /* INCLUDE_CHIA_DISKSORT_H_ */
