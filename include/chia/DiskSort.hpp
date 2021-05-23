/*
 * DiskSort.hpp
 *
 *  Created on: May 23, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_DISKSORT_HPP_
#define INCLUDE_CHIA_DISKSORT_HPP_

#include <chia/DiskSort.h>

#include <algorithm>
#include <unordered_map>


template<typename T, typename Sort, typename Key>
DiskSort<T, Sort, Key>::DiskSort(int key_size, int log_num_buckets, std::string file_prefix)
	:	key_size(key_size),
		log_num_buckets(log_num_buckets),
		buckets(1 << log_num_buckets)
{
	for(size_t i = 0; i < buckets.size(); ++i) {
		auto& bucket = buckets[i];
		bucket.file_name = file_prefix + ".sort_bucket." + std::to_string(i);
		bucket.file = ::fopen(bucket.file_name.c_str(), "wb");
		if(!bucket.file) {
			throw std::runtime_error("fopen() failed");
		}
	}
}

template<typename T, typename Sort, typename Key>
void DiskSort<T, Sort, Key>::add(const T& entry)
{
	const size_t index = Key(entry) >> (key_size - log_num_buckets);
	if(index >= buckets.size()) {
		throw std::logic_error("index out of range");
	}
	auto& bucket = buckets[index];
	if(::fwrite(&entry, 1, sizeof(entry), bucket.file) != sizeof(entry)) {
		throw std::runtime_error("fwrite() failed");
	}
	bucket.num_entries++;
}

template<typename T, typename Sort, typename Key>
std::vector<std::vector<T>>
DiskSort<T, Sort, Key>::read_bucket(const size_t index, const size_t M)
{
	if(index >= buckets.size()) {
		throw std::logic_error("index out of range");
	}
	auto& bucket = buckets[index];
	auto& file = bucket.file;
	if(file) {
		::fclose(file);
	}
	file = ::fopen(bucket.file_name.c_str(), "rb");
	if(!file) {
		throw std::runtime_error("fopen() failed");
	}
	std::unordered_map<size_t, std::vector<T>> table;
	table.reserve(4096);
	
	for(size_t i = 0; i < bucket.num_entries; ++i) {
		T entry;
		if(::fread(&entry, 1, sizeof(entry), file) != sizeof(entry)) {
			throw std::runtime_error("fread() failed");
		}
		auto& block = table[Key(entry) / M];
		if(block.empty()) {
			block.reserve(M);
		}
		block.push_back(entry);
	}
	
	std::vector<std::pair<size_t, std::vector<T>>> list;
	list.reserve(table.size());
	for(auto& entry : table) {
		list.emplace_back(entry.first, std::move(entry.second));
	}
	table.clear();
	
	std::sort(list.begin(), list.end(),
		[](	const std::pair<size_t, std::vector<T>>& lhs,
			const std::pair<size_t, std::vector<T>>& rhs) -> bool {
			return Sort{}(lhs.first, rhs.first);
		});
	
	std::vector<std::vector<T>> out;
	out.reserve(list.size());
	for(auto& entry : list) {
		out.emplace_back(std::move(entry.second));
	}
	list.clear();
	
	// TODO: openmp
	for(size_t i = 0; i < out.size(); ++i) {
		auto& block = out[i];
		std::sort(list.begin(), list.end(),
			[](const T& lhs, const T& rhs) -> bool {
				return Sort{}(Key(lhs), Key(rhs));
			});
	}
	return out;
}

template<typename T, typename Sort, typename Key>
void DiskSort<T, Sort, Key>::clear()
{
	for(auto& bucket : buckets) {
		::fclose(bucket.file);
		bucket.file = nullptr;
		std::remove(bucket.file_name.c_str());
	}
}


#endif /* INCLUDE_CHIA_DISKSORT_HPP_ */
