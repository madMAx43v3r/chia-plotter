/*
 * phase1.h
 *
 *  Created on: May 25, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_PHASE1_H_
#define INCLUDE_CHIA_PHASE1_H_

#include <chia/chia.h>
#include <chia/entries.h>
#include <chia/DiskSort.h>
#include <chia/util.hpp>

#include <array>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstring>


namespace phase1 {

struct input_t {
	int k = 32;
	std::array<uint8_t, 32> id = {};
	std::vector<uint8_t> memo;
	std::string plot_name;
};

struct entry_1 {
	uint64_t y;			// 38 bit
	uint32_t x;			// 32 bit
	
	static constexpr uint32_t pos = 0;		// dummy
	static constexpr uint16_t off = 0;		// dummy
	static constexpr size_t disk_size = 9;
	
	size_t read(const uint8_t* buf) {
		y = 0;
		memcpy(&y, buf, 5);
		memcpy(&x, buf + 5, 4);
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &y, 5);
		memcpy(buf + 5, &x, 4);
		return disk_size;
	}
};

struct entry_x {
	uint64_t y;			// 38 bit
	uint32_t pos;		// 32 bit
	uint16_t off;		// 10 bit
};

template<int N>
struct entry_xm : entry_x {
	std::array<uint8_t, N * 4> meta;
	
	static constexpr size_t disk_size = 10 + N * 4;
	
	size_t read(const uint8_t* buf) {
		memcpy(&y, buf, 5);
		y &= 0x3FFFFFFFFFull;
		off = 0;
		off |= buf[4] >> 6;
		off |= uint16_t(buf[5]) << 2;
		memcpy(&pos, buf + 6, 4);
		memcpy(meta.data(), buf + 10, sizeof(meta));
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &y, 5);
		buf[4] = (off << 6) | (buf[4] & 0x3F);
		buf[5] = off >> 2;
		memcpy(buf + 6, &pos, 4);
		memcpy(buf + 10, meta.data(), sizeof(meta));
		return disk_size;
	}
};

typedef entry_xm<2> entry_2;
typedef entry_xm<4> entry_3;
typedef entry_xm<4> entry_4;
typedef entry_xm<3> entry_5;
typedef entry_xm<2> entry_6;

struct entry_7 {
	uint32_t y;			// 32 bit
	uint32_t pos;		// 32 bit
	uint16_t off;		// 10 bit
	
	static constexpr size_t disk_size = 10;
	
	void assign(const entry_7& entry) {
		*this = entry;
	}
	size_t read(const uint8_t* buf) {
		memcpy(&y, buf, 4);
		memcpy(&pos, buf + 4, 4);
		memcpy(&off, buf + 8, 2);
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &y, 4);
		memcpy(buf + 4, &pos, 4);
		memcpy(buf + 8, &off, 2);
		return disk_size;
	}
};

struct tmp_entry_1 {
	uint32_t x;			// 32 bit
	
	static constexpr size_t disk_size = 4;
	
	void assign(const entry_1& entry) {
		x = entry.x;
	}
	size_t read(const uint8_t* buf) {
		memcpy(&x, buf, 4);
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &x, 4);
		return disk_size;
	}
};

struct tmp_entry_x {
	uint32_t pos;		// 32 bit
	uint16_t off;		// 10 bit
	
	static constexpr size_t disk_size = 6;
	
	void assign(const entry_x& entry) {
		pos = entry.pos;
		off = entry.off;
	}
	size_t read(const uint8_t* buf) {
		memcpy(&pos, buf, 4);
		memcpy(&off, buf + 4, 2);
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &pos, 4);
		memcpy(buf + 4, &off, 2);
		return disk_size;
	}
};

template<typename T>
struct get_y {
	uint64_t operator()(const T& entry) {
		return entry.y;
	}
};

template<typename T>
struct get_meta {
	void operator()(const T& entry, uint128_t* value) {
		*value = 0;
		memcpy(value, entry.meta.data(), sizeof(entry.meta));
	}
};

template<>
struct get_meta<entry_1> {
	void operator()(const entry_1& entry, uint128_t* value) {
		*value = entry.x;
	}
};

template<typename T>
struct set_meta {
	void operator()(T& entry, const uint128_t value, const size_t num_bytes) {
		entry.meta = {};
		memcpy(entry.meta.data(), &value, num_bytes);
	}
};

template<>
struct set_meta<entry_7> {
	void operator()(entry_7& entry, const uint128_t value, const size_t num_bytes) {
		// no meta data
	}
};

template<typename T>
struct match_t {
	T left;
	T right;
	uint32_t pos = 0;
	uint16_t off = 0;
};

typedef DiskSort<entry_1, get_y<entry_1>> DiskSort1;
typedef DiskSort<entry_2, get_y<entry_2>> DiskSort2;
typedef DiskSort<entry_3, get_y<entry_3>> DiskSort3;
typedef DiskSort<entry_4, get_y<entry_4>> DiskSort4;
typedef DiskSort<entry_5, get_y<entry_5>> DiskSort5;
typedef DiskSort<entry_6, get_y<entry_6>> DiskSort6;
typedef DiskSort<entry_7, get_y<entry_7>> DiskSort7;

struct output_t {
	input_t params;
	std::array<table_t, 7> table;
};


} // phase1

#endif /* INCLUDE_CHIA_PHASE1_H_ */
