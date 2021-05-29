/*
 * DiskSort.hpp
 *
 *  Created on: May 23, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_DISKSORT_HPP_
#define INCLUDE_CHIA_DISKSORT_HPP_

#include <chia/DiskSort.h>

#include <map>
#include <algorithm>
#include <unordered_map>


template<typename T, typename Key>
DiskSort<T, Key>::DiskSort(	int key_size, int log_num_buckets, int num_threads,
							std::string file_prefix, int num_threads_read)
	:	key_size(key_size),
		log_num_buckets(log_num_buckets),
		bucket_key_shift(key_size - log_num_buckets),
		num_threads(num_threads),
		num_threads_read(num_threads_read),
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
	const size_t index = Key{}(entry) >> bucket_key_shift;
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
void DiskSort<T, Key>::read(Processor<std::vector<T>>* output)
{
	ThreadPool<std::vector<T>, std::vector<T>> sort_pool(
			std::bind(&DiskSort::sort_block, this, std::placeholders::_1, std::placeholders::_2),
			output, num_threads, "Disk/sort");
	
	Thread<std::vector<std::vector<T>>> sort_thread(
			std::bind(&DiskSort::sort_bucket, this, std::placeholders::_1, &sort_pool), "Disk/sort");
	
	ThreadPool<size_t, std::vector<std::vector<T>>> read_pool(
			std::bind(&DiskSort::read_bucket, this, std::placeholders::_1, std::placeholders::_2),
			&sort_thread, num_threads_read, "Disk/read");
	
	for(size_t i = 0; i < buckets.size(); ++i) {
		read_pool.take_copy(i);
	}
	read_pool.wait();
	sort_thread.wait();
	sort_pool.wait();
}

template<typename T, typename Key>
void DiskSort<T, Key>::read_bucket(size_t& index, std::vector<std::vector<T>>& out)
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
	
	const int key_shift = bucket_key_shift - log_num_buckets;
	if(key_shift < 0) {
		throw std::logic_error("key_shift < 0");
	}
	std::unordered_map<size_t, std::vector<T>> table;
	table.reserve(size_t(1) << log_num_buckets);
	
	static constexpr size_t N = 65536;
	uint8_t buffer[N * T::disk_size];
	
	for(size_t i = 0; i < bucket.num_entries;)
	{
		const size_t num_entries = std::min(N, bucket.num_entries - i);
		if(fread(buffer, T::disk_size, num_entries, file) != num_entries) {
			throw std::runtime_error("fread() failed");
		}
		for(size_t k = 0; k < num_entries; ++k) {
			T entry;
			entry.read(buffer + k * T::disk_size);
			
			auto& block = table[Key{}(entry) >> key_shift];
			if(block.empty()) {
				block.reserve(avg_block_size * 1.1);
			}
			block.push_back(entry);
		}
		i += num_entries;
	}
	
	std::map<size_t, std::vector<T>> sorted;
	for(auto& entry : table) {
		sorted.emplace(entry.first, std::move(entry.second));
	}
	table.clear();
	
	out.reserve(sorted.size());
	for(auto& entry : sorted) {
		out.emplace_back(std::move(entry.second));
	}
	if(!sorted.empty()) {
		avg_block_size = double(out.size()) / sorted.size();
	}
//	std::cout << "bucket " << index << ": avg block size = " << avg_block_size << std::endl;
}

template<typename T, typename Key>
void DiskSort<T, Key>::sort_bucket(std::vector<std::vector<T>>& input, Processor<std::vector<T>>* sort)
{
	for(auto& block : input) {
		sort->take(block);
	}
}

template<typename T, typename Key>
void DiskSort<T, Key>::sort_block(std::vector<T>& input, std::vector<T>& out)
{
	std::sort(input.begin(), input.end(),
		[](const T& lhs, const T& rhs) -> bool {
			return Key{}(lhs) < Key{}(rhs);
		});
	out = std::move(input);
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
void DiskSort<T, Key>::close()
{
	for(auto& bucket : buckets) {
		fclose(bucket.file);
		bucket.file = nullptr;
		std::remove(bucket.file_name.c_str());
	}
	buckets.clear();
}


#endif /* INCLUDE_CHIA_DISKSORT_HPP_ */
