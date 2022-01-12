/*
 * DiskSort.hpp
 *
 *  Created on: May 23, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_DISKSORT_HPP_
#define INCLUDE_CHIA_DISKSORT_HPP_

#include <chia/DiskSort.h>
#include <chia/util.hpp>

#include <map>
#include <algorithm>
#include <unordered_map>


template<typename T, typename Key>
void DiskSort<T, Key>::bucket_t::open(const char* mode)
{
	if(file) {
		fclose(file);
	}
	file = fopen(file_name.c_str(), mode);
	if(!file) {
		throw std::runtime_error("fopen() failed with: " + std::string(std::strerror(errno)));
	}
}

template<typename T, typename Key>
void DiskSort<T, Key>::bucket_t::write(const void* data, size_t count)
{
	std::lock_guard<std::mutex> lock(mutex);
	if(file) {
		if(fwrite(data, T::disk_size, count, file) != count) {
			throw std::runtime_error("fwrite() failed with: " + std::string(std::strerror(errno)));
		}
		num_entries += count;
	}
}

template<typename T, typename Key>
void DiskSort<T, Key>::bucket_t::close()
{
	if(file) {
		if(fclose(file)) {
			throw std::runtime_error("fclose() failed with: " + std::string(std::strerror(errno)));
		}
		file = nullptr;
	}
}

template<typename T, typename Key>
void DiskSort<T, Key>::bucket_t::remove()
{
	close();
	std::remove(file_name.c_str());
}

template<typename T, typename Key>
DiskSort<T, Key>::WriteCache::WriteCache(DiskSort* disk, int key_shift, int num_buckets)
	:	disk(disk), key_shift(key_shift), buckets(num_buckets)
{
}

template<typename T, typename Key>
void DiskSort<T, Key>::WriteCache::add(const T& entry)
{
	const size_t index = Key{}(entry) >> key_shift;
	if(index >= buckets.size()) {
		throw std::logic_error("bucket index out of range: "
				+ std::to_string(index) + " >= " + std::to_string(buckets.size()));
	}
	auto& buffer = buckets[index];
	if(buffer.count >= buffer.capacity) {
		disk->write(index, buffer.data, buffer.count);
		buffer.count = 0;
	}
	entry.write(buffer.entry_at(buffer.count));
	buffer.count++;
}

template<typename T, typename Key>
void DiskSort<T, Key>::WriteCache::flush()
{
	for(size_t index = 0; index < buckets.size(); ++index) {
		auto& buffer = buckets[index];
		if(buffer.count) {
			disk->write(index, buffer.data, buffer.count);
			buffer.count = 0;
		}
	}
}

template<typename T, typename Key>
DiskSort<T, Key>::DiskSort(	int key_size, int log_num_buckets,
							std::string file_prefix, bool read_only)
	:	key_size(key_size),
		log_num_buckets(log_num_buckets),
		bucket_key_shift(key_size - log_num_buckets),
		keep_files(read_only),
		is_finished(read_only),
		cache(this, key_size - log_num_buckets, 1 << log_num_buckets),
		buckets(1 << log_num_buckets)
{
	for(size_t i = 0; i < buckets.size(); ++i) {
		auto& bucket = buckets[i];
		bucket.file_name = file_prefix + ".sort_bucket_" + std::to_string(i) + ".tmp";
		if(read_only) {
			bucket.num_entries = get_file_size(bucket.file_name.c_str()) / T::disk_size;
		} else {
			bucket.open("wb");
		}
	}
}

template<typename T, typename Key>
void DiskSort<T, Key>::add(const T& entry)
{
	cache.add(entry);
}

template<typename T, typename Key>
void DiskSort<T, Key>::write(size_t index, const void* data, size_t count)
{
	if(is_finished) {
		throw std::logic_error("read only");
	}
	if(index >= buckets.size()) {
		throw std::logic_error("bucket index out of range");
	}
	buckets[index].write(data, count);
}

template<typename T, typename Key>
std::shared_ptr<typename DiskSort<T, Key>::WriteCache> DiskSort<T, Key>::add_cache()
{
	return std::make_shared<WriteCache>(this, bucket_key_shift, buckets.size());
}

template<typename T, typename Key>
void DiskSort<T, Key>::read(Processor<std::pair<std::vector<T>, size_t>>* output,
							int num_threads, int num_threads_read)
{
	if(num_threads_read < 0) {
		num_threads_read = std::max(num_threads / 2, 2);
	}
	
	ThreadPool<	std::pair<std::vector<T>, size_t>,
				std::pair<std::vector<T>, size_t>> sort_pool(
		[](std::pair<std::vector<T>, size_t>& input, std::pair<std::vector<T>, size_t>& out, size_t&) {
			std::sort(input.first.begin(), input.first.end(),
				[](const T& lhs, const T& rhs) -> bool {
					return Key{}(lhs) < Key{}(rhs);
				});
			out = std::move(input);
		}, output, num_threads, "Disk/sort");
	
	Thread<std::vector<std::pair<std::vector<T>, size_t>>> sort_thread(
		[&sort_pool](std::vector<std::pair<std::vector<T>, size_t>>& input) {
			for(auto& block : input) {
				sort_pool.take(block);
			}
		}, "Disk/sort");
	
	ThreadPool<	std::pair<size_t, size_t>,
				std::vector<std::pair<std::vector<T>, size_t>>,
				read_buffer_t<T>> read_pool(
		std::bind(&DiskSort::read_bucket, this,
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		&sort_thread, num_threads_read, "Disk/read");
	
	uint64_t offset = 0;
	for(size_t i = 0; i < buckets.size(); ++i) {
		read_pool.take_copy(std::make_pair(i, offset));
		offset += buckets[i].num_entries;
	}
	read_pool.close();
	sort_thread.close();
	sort_pool.close();
}

template<typename T, typename Key>
void DiskSort<T, Key>::read_bucket(	std::pair<size_t, size_t>& index,
									std::vector<std::pair<std::vector<T>, size_t>>& out,
									read_buffer_t<T>& buffer)
{
	auto& bucket = buckets[index.first];
	bucket.open("rb");
	
	const int key_shift = bucket_key_shift - log_num_buckets;
	if(key_shift < 0) {
		throw std::logic_error("key_shift < 0");
	}
	std::unordered_map<size_t, std::vector<T>> table;
	table.reserve(size_t(1) << log_num_buckets);
	
	for(size_t i = 0; i < bucket.num_entries;)
	{
		const size_t num_entries = std::min(buffer.capacity, bucket.num_entries - i);
		if(fread(buffer.data, T::disk_size, num_entries, bucket.file) != num_entries) {
			throw std::runtime_error("fread() failed with: " + std::string(std::strerror(errno)));
		}
		for(size_t k = 0; k < num_entries; ++k) {
			T entry;
			entry.read(buffer.entry_at(k));
			
			auto& block = table[Key{}(entry) >> key_shift];
			if(block.empty()) {
				block.reserve((bucket.num_entries >> log_num_buckets) * 1.1);
			}
			block.push_back(entry);
		}
		i += num_entries;
	}
	if(!keep_files) {
		bucket.remove();
	}
	
	std::map<size_t, std::vector<T>> sorted;
	for(auto& entry : table) {
		sorted.emplace(entry.first, std::move(entry.second));
	}
	table.clear();
	
	out.reserve(sorted.size());
	uint64_t offset = index.second;
	for(auto& entry : sorted) {
		const auto count = entry.second.size();
		out.emplace_back(std::move(entry.second), offset);
		offset += count;
	}
}

template<typename T, typename Key>
void DiskSort<T, Key>::finish()
{
	cache.flush();
	for(auto& bucket : buckets) {
		bucket.close();
	}
	is_finished = true;
}

template<typename T, typename Key>
void DiskSort<T, Key>::close()
{
	for(auto& bucket : buckets) {
		bucket.close();
		if(!keep_files) {
			bucket.remove();
		}
	}
	buckets.clear();
}


#endif /* INCLUDE_CHIA_DISKSORT_HPP_ */
