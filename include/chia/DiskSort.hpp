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


template<typename T, typename Key>
DiskSort<T, Key>::DiskSort(int key_size, int log_num_buckets, std::string file_prefix)
	:	key_size(key_size),
		log_num_buckets(log_num_buckets),
		buckets(1 << log_num_buckets)
{
	for(size_t i = 0; i < buckets.size(); ++i) {
		auto& bucket = buckets[i];
		bucket.file_name = file_prefix + ".sort_bucket_" + std::to_string(i) + ".tmp";
		bucket.file = fopen(bucket.file_name.c_str(), "wb");
		if(!bucket.file) {
			throw std::runtime_error("fopen() failed");
		}
	}
}

template<typename T, typename Key>
void DiskSort<T, Key>::add(const T& entry)
{
	if(is_finished) {
		throw std::logic_error("read only");
	}
	const size_t index = Key{}(entry) >> (key_size - log_num_buckets);
	if(index >= buckets.size()) {
		throw std::logic_error("index out of range");
	}
	auto& bucket = buckets[index];
	if(bucket.offset + T::disk_size > sizeof(bucket.buffer)) {
		bucket.flush();
	}
	bucket.offset += entry.write(bucket.buffer + bucket.offset);
	bucket.num_entries++;
}

template<typename T, typename Key>
void DiskSort<T, Key>::bucket_t::flush()
{
	if(fwrite(buffer, 1, offset, file) != offset) {
		throw std::runtime_error("fwrite() failed");
	}
	offset = 0;
}

template<typename T, typename Key>
void DiskSort<T, Key>::read(Thread<output_t>& output, size_t M)
{
	Thread<std::pair<std::vector<std::vector<T>>, bool>> sort(
			std::bind(&DiskSort::sort_bucket, this, std::placeholders::_1, &output), "DiskSort");
	for(size_t i = 0; i < buckets.size(); ++i) {
		read_bucket(i, &sort, &output, M);
	}
}

template<typename T, typename Key>
void DiskSort<T, Key>::read_bucket(	const size_t index,
									Thread<std::pair<std::vector<std::vector<T>>, bool>>* sort,
									Thread<output_t>* output, const size_t M)
{
	auto& bucket = buckets[index];
	auto& file = bucket.file;
	if(file) {
		fclose(file);
	}
	file = fopen(bucket.file_name.c_str(), "rb");
	if(!file) {
		throw std::runtime_error("fopen() failed");
	}
	std::unordered_map<size_t, std::vector<T>> table;
	table.reserve(4096);
	
	constexpr size_t N = 1024;
	uint8_t buffer[N * T::disk_size];
	
	for(size_t i = 0; i < bucket.num_entries;) {
		const size_t num_entries = std::min(N, bucket.num_entries - i);
		if(fread(buffer, T::disk_size, num_entries, file) != num_entries) {
			throw std::runtime_error("fread() failed");
		}
		for(size_t k = 0; k < num_entries; ++k) {
			T entry;
			entry.read(buffer + k * T::disk_size);
			
			auto& block = table[Key{}(entry) / M];
			if(block.empty()) {
				block.reserve(M);
			}
			block.push_back(entry);
		}
		i += num_entries;
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
			return lhs.first < rhs.first;
		});
	
	std::vector<std::vector<T>> blocks;
	blocks.reserve(list.size());
	for(auto& entry : list) {
		blocks.emplace_back(std::move(entry.second));
	}
	list.clear();
	
	std::pair<std::vector<std::vector<T>>, bool> out;
	out.first = std::move(blocks);
	out.second = index + 1 == buckets.size();
	sort->take(out);
}

template<typename T, typename Key>
void DiskSort<T, Key>::sort_bucket(	std::pair<std::vector<std::vector<T>>, bool>& input,
									Thread<output_t>* output)
{
	for(size_t i = 0; i < input.first.size(); ++i) {
		auto& block = input.first[i];
		std::sort(block.begin(), block.end(),
			[](const T& lhs, const T& rhs) -> bool {
				return Key{}(lhs) < Key{}(rhs);
			});
		output_t out;
		out.last_block = i + 1 == input.first.size();
		out.last_bucket = input.second;
		out.block = std::move(block);
		output->take(out);
	}
}

template<typename T, typename Key>
void DiskSort<T, Key>::finish() {
	for(auto& bucket : buckets) {
		bucket.flush();
		fflush(bucket.file);
	}
	is_finished = true;
}

template<typename T, typename Key>
void DiskSort<T, Key>::clear()
{
	for(auto& bucket : buckets) {
		fclose(bucket.file);
		bucket.file = nullptr;
		std::remove(bucket.file_name.c_str());
	}
}


#endif /* INCLUDE_CHIA_DISKSORT_HPP_ */
