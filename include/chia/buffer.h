/*
 * buffer.h
 *
 *  Created on: Jun 7, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_BUFFER_H_
#define INCLUDE_CHIA_BUFFER_H_

#include <chia/settings.h>


template<typename T>
struct byte_buffer_t {
	size_t count = 0;
	const size_t capacity;
	uint8_t* data = nullptr;
	static constexpr size_t entry_size = T::disk_size;
	
	byte_buffer_t(const size_t capacity) : capacity(capacity) {
		data = new uint8_t[capacity * entry_size];
	}
	~byte_buffer_t() {
		delete [] data;
	}
	uint8_t* entry_at(const size_t i) {
		return data + i * entry_size;
	}
	byte_buffer_t(byte_buffer_t&) = delete;
	byte_buffer_t& operator=(byte_buffer_t&) = delete;
};

template<typename T>
struct read_buffer_t : byte_buffer_t<T> {
	read_buffer_t() : byte_buffer_t<T>(g_read_chunk_size) {}
};

template<typename T>
struct write_buffer_t : byte_buffer_t<T> {
	write_buffer_t() : byte_buffer_t<T>(g_write_chunk_size) {}
};


#endif /* INCLUDE_CHIA_BUFFER_H_ */
