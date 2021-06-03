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
private:
	template<size_t N>
	struct buffer_t {
		size_t count = 0;
		uint8_t buffer[N * T::disk_size];
		static constexpr size_t max_count = N;
	};
	
	struct bucket_t {
		FILE* file = nullptr;
		std::mutex mutex;
		std::string file_name;
		size_t num_entries = 0;
		
		void open(const char* mode);
		void write(const void* data, size_t count);
		void close();
		void remove();
	};
	
public:
	class WriteCache {
	public:
		WriteCache(DiskSort* disk, int key_shift, int num_buckets);
		~WriteCache() { flush(); }
		void add(const T& entry);
		void flush();
	private:
		DiskSort* disk = nullptr;
		const int key_shift = 0;
		std::vector<buffer_t<4096>> buckets;
	};
	
	DiskSort(	int key_size, int log_num_buckets,
				std::string file_prefix, bool read_only = false);
	
	~DiskSort() {
		close();
	}
	
	DiskSort(DiskSort&) = delete;
	DiskSort& operator=(DiskSort&) = delete;
	
	void read(Processor<std::vector<T>>* output, int num_threads, int num_threads_read = 2);
	
	void finish();
	
	void close();
	
	void add(const T& entry);
	
	// thread safe
	void write(size_t index, const void* data, size_t count);
	
	std::shared_ptr<WriteCache> add_cache();
	
	size_t num_buckets() const {
		return buckets.size();
	}
	
	void set_keep_files(bool enable) {
		keep_files = enable;
	}
	
private:
	void read_bucket(size_t& index, std::vector<std::vector<T>>& out);
	
private:
	const int key_size = 0;
	const int log_num_buckets = 0;
	const int bucket_key_shift = 0;
	
	bool keep_files = false;
	bool is_finished = false;
	
	WriteCache cache;
	std::vector<bucket_t> buckets;
	
};



#endif /* INCLUDE_CHIA_DISKSORT_H_ */
