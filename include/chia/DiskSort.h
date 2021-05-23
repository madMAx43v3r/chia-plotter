/*
 * DiskSort.h
 *
 *  Created on: May 23, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_DISKSORT_H_
#define INCLUDE_CHIA_DISKSORT_H_

#include <chia/Thread.h>

#include <vector>
#include <string>
#include <cstdio>
#include <cstddef>
#include <memory>
#include <functional>


template<typename T, typename Sort, typename Key>
class DiskSort {
public:
	DiskSort(int key_size, int log_num_buckets, std::string file_prefix);
	
	~DiskSort() {
		clear();
	}
	
	void read(Thread<std::vector<T>>& output, size_t M);
	
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
		char buffer[262144];
		void flush();
	};
	
	void read_bucket(	const size_t index,
						Thread<std::vector<std::vector<T>>>& sort,
						Thread<std::vector<T>>& output, const size_t M);
	
	void sort_bucket_test(std::vector<std::vector<T>>& blocks) {}
	
	void sort_bucket(std::vector<std::vector<T>>& blocks, Thread<std::vector<T>>* output);
	
private:
	int key_size = 0;
	int log_num_buckets = 0;
	bool is_finished = false;
	std::vector<bucket_t> buckets;
	
};



#endif /* INCLUDE_CHIA_DISKSORT_H_ */
