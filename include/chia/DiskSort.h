/*
 * DiskSort.h
 *
 *  Created on: May 23, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_DISKSORT_H_
#define INCLUDE_CHIA_DISKSORT_H_

#include <chia/buffer.h>
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
		std::vector<write_buffer_t<T>> buckets;
	};
	
	DiskSort(	int key_size, int log_num_buckets,
				std::string file_prefix, bool read_only = false);
	
	~DiskSort() {
		close();
	}
	
	DiskSort(DiskSort&) = delete;
	DiskSort& operator=(DiskSort&) = delete;
	
	void read(	Processor<std::pair<std::vector<T>, size_t>>* output,
				int num_threads, int num_threads_read = -1);
	
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
	void read_bucket(	std::pair<size_t, size_t>& index,
						std::vector<std::pair<std::vector<T>, size_t>>& out,
						read_buffer_t<T>& buffer);
	
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
