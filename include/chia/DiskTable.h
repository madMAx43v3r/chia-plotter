/*
 * Table.h
 *
 *  Created on: May 29, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_DISKTABLE_H_
#define INCLUDE_CHIA_DISKTABLE_H_

#include <chia/ThreadPool.h>

#include <cstdio>


template<typename T>
class DiskTable {
private:
	struct local_t {
		FILE* file = nullptr;
		uint8_t* buffer = nullptr;
	};
	
public:
	DiskTable(std::string file_name, size_t num_entries = 0)
		:	file_name(file_name),
			num_entries(num_entries)
	{
		if(!num_entries) {
			file_out = fopen(file_name.c_str(), "wb");
		}
	}
	
	~DiskTable() {
		close();
	}
	
	DiskTable(DiskTable&) = delete;
	DiskTable& operator=(DiskTable&) = delete;
	
	void read(	Processor<std::pair<std::vector<T>, size_t>>* output,
				int num_threads_read = 2, const size_t block_size = 65536) const
	{
		ThreadPool<std::pair<size_t, size_t>, std::pair<std::vector<T>, size_t>, local_t> pool(
			std::bind(&DiskTable::read_block, this,
					std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			output, num_threads_read, "Table/read");
		
		for(size_t i = 0; i < pool.num_threads(); ++i)
		{
			FILE* file = fopen(file_name.c_str(), "rb");
			if(!file) {
				throw std::runtime_error("fopen() failed");
			}
			auto& local = pool.get_local(i);
			local.file = file;
			local.buffer = new uint8_t[block_size * T::disk_size];
		}
		const size_t num_blocks = num_entries / block_size;
		const size_t left_over = num_entries % block_size;
		
		size_t offset = 0;
		for(size_t i = 0; i < num_blocks; ++i) {
			pool.take_copy(std::make_pair(offset, block_size));
			offset += block_size;
		}
		if(left_over) {
			pool.take_copy(std::make_pair(offset, left_over));
		}
		pool.wait();
		
		for(size_t i = 0; i < pool.num_threads(); ++i)
		{
			auto& local = pool.get_local(i);
			fclose(local.file);
			delete [] local.buffer;
		}
	}
	
	// NOT thread-safe
	void write(const T& entry) {
		if(cache_count >= N) {
			flush();
		}
		entry.write(cache + cache_count * T::disk_size);
		cache_count++;
	}
	
	void flush() {
		if(fwrite(cache, T::disk_size, cache_count, file_out) != cache_count) {
			throw std::runtime_error("fwrite() failed");
		}
		num_entries += cache_count;
		cache_count = 0;
	}
	
	void close() {
		if(file_out) {
			flush();
			fclose(file_out);
			file_out = nullptr;
		}
	}
	
private:
	void read_block(std::pair<size_t, size_t>& param,
					std::pair<std::vector<T>, size_t>& out, local_t& local) const
	{
		if(int err = fseek(local.file, param.first * T::disk_size, SEEK_SET)) {
			throw std::runtime_error("fseek() failed");
		}
		if(fread(local.buffer, T::disk_size, param.second, local.file) != param.second) {
			throw std::runtime_error("fread() failed");
		}
		auto& entries = out.first;
		entries.resize(param.second);
		for(size_t k = 0; k < param.second; ++k) {
			entries[k].read(local.buffer + k * T::disk_size);
		}
		out.second = param.first;
	}
	
private:
	std::string file_name;
	size_t num_entries;
	
	static constexpr size_t N = 4096;
	uint8_t cache[N * T::disk_size];
	size_t cache_count = 0;
	FILE* file_out = nullptr;
	
};


#endif /* INCLUDE_CHIA_DISKTABLE_H_ */
