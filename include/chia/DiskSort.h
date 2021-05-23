/*
 * DiskSort.h
 *
 *  Created on: May 23, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_DISKSORT_H_
#define INCLUDE_CHIA_DISKSORT_H_

#include <vector>
#include <string>
#include <cstdio>
#include <cstddef>


template<typename T, typename Sort, typename Key>
class DiskSort {
public:
	DiskSort(int key_size, int log_num_buckets, std::string file_prefix);
	
	~DiskSort();
	
	std::vector<std::vector<T>> read_bucket(const size_t index) {
		return read_bucket(index, 4096);
	}
	
	std::vector<std::vector<T>> read_bucket(const size_t index, const size_t M);
	
	void clear();
	
	void add(const T& entry);
	
	size_t num_buckets() const {
		return buckets.size();
	}
	
private:
	struct bucket_t {
		FILE* file = nullptr;
		std::string file_name;
		size_t num_entries = 0;
	};
	
private:
	int key_size = 0;
	int log_num_buckets = 0;
	std::vector<bucket_t> buckets;
	
};



#endif /* INCLUDE_CHIA_DISKSORT_H_ */
