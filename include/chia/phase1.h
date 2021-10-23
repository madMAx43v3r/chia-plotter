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
	uint64_t y;			// 38 bit / 40 bit
	uintkx_t x;			// 32 bit / 34 bit
	
	static constexpr uintkx_t pos = 0;		// dummy
	static constexpr uint16_t off = 0;		// dummy
	static constexpr size_t disk_size = 5 + KBYTES;
	
	size_t read(const uint8_t* buf) {
		y = 0;
		memcpy(&y, buf, 5);
		if(sizeof(x) > KBYTES) {
			x = 0;
		}
		memcpy(&x, buf + 5, KBYTES);		// 32 bit / 40 bit
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &y, 5);
		memcpy(buf + 5, &x, KBYTES);
		return disk_size;
	}
};

struct entry_x {
	uint64_t y;			// 38 bit / 40 bit
	uintkx_t pos;		// 32 bit / 35 bit
	uint16_t off;		// 10 bit
};

template<int N>
struct entry_xm : entry_x {
	std::array<uint8_t, N> meta;

#ifdef CHIA_K34
	static constexpr size_t disk_size = 11 + N;
	
	size_t read(const uint8_t* buf) {
		y = 0;
		memcpy(&y, buf, 5);
		memcpy(&pos, buf + 5, 5);
		pos &= 0x3FFFFFFFFF;				// 38 bit
		memcpy(&off, buf + 9, 2);
		off >>= 6;
		memcpy(meta.data(), buf + 11, meta.size());
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &y, 5);
		memcpy(buf + 5, &pos, 5);
		{
			const auto tmp = (off << 6) | buf[9];
			memcpy(buf + 9, &tmp, 2);
		}
		memcpy(buf + 11, meta.data(), meta.size());
		return disk_size;
	}
#else
	static constexpr size_t disk_size = 10 + N;

	size_t read(const uint8_t* buf) {
		memcpy(&y, buf, 5);
		y &= 0x3FFFFFFFFFull;
		off = 0;
		off |= buf[4] >> 6;
		off |= uint16_t(buf[5]) << 2;
		memcpy(&pos, buf + 6, 4);
		memcpy(meta.data(), buf + 10, meta.size());
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &y, 5);
		buf[4] = (off << 6) | (buf[4] & 0x3F);
		buf[5] = off >> 2;
		memcpy(buf + 6, &pos, 4);
		memcpy(buf + 10, meta.data(), meta.size());
		return disk_size;
	}
#endif
};

#ifdef CHIA_K34
typedef entry_xm<9>  entry_2;
typedef entry_xm<17> entry_3;
typedef entry_xm<17> entry_4;
typedef entry_xm<13> entry_5;
typedef entry_xm<9>  entry_6;
#else
typedef entry_xm<8>  entry_2;
typedef entry_xm<16> entry_3;
typedef entry_xm<16> entry_4;
typedef entry_xm<12> entry_5;
typedef entry_xm<8>  entry_6;
#endif

struct entry_7 {
	uintkx_t y;			// 32 bit / 34 bit
	uintkx_t pos;		// 32 bit / 35 bit
	uint16_t off;		// 10 bit
	
#ifdef CHIA_K34
	static constexpr size_t disk_size = 11;

	void assign(const entry_7& entry) {
		*this = entry;
	}
	size_t read(const uint8_t* buf) {
		memcpy(&y, buf, 5);
		y &= 0xFFFFFFFFF;				// 36 bit
		memcpy(&pos, buf + 4, 5);
		pos >>= 4;
		pos &= 0xFFFFFFFFF;				// 36 bit
		memcpy(&off, buf + 9, 2);
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &y, 5);
		const auto tmp = (pos << 4) | buf[4];
		memcpy(buf + 4, &tmp, 5);
		memcpy(buf + 9, &off, 2);
		return disk_size;
	}
#else
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
#endif
};

struct tmp_entry_1 {
	uintkx_t x;			// 32 bit / 34 bit
	
	static constexpr size_t disk_size = KBYTES;
	
	void assign(const entry_1& entry) {
		x = entry.x;
	}
	size_t read(const uint8_t* buf) {
		if(sizeof(x) > KBYTES) {
			x = 0;
		}
		memcpy(&x, buf, KBYTES);
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &x, KBYTES);
		return disk_size;
	}
};

struct tmp_entry_x {
	uintkx_t pos;		// 32 bit / 35 bit
	uint16_t off;		// 10 bit
	
	static constexpr size_t disk_size = 6;
	
	void assign(const entry_x& entry) {
		pos = entry.pos;
		off = entry.off;
	}
	size_t read(const uint8_t* buf) {
		memcpy(&pos, buf, KBYTES);
		pos &= 0x3FFFFFFFFF;			// 38 bit
		memcpy(&off, buf + 4, 2);
		off >>= 6;
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &pos, KBYTES);
		if(KBYTES < 5) {
			buf[4] = 0;
		}
		const auto tmp = (off << 6) | buf[4];
		memcpy(buf + 4, &tmp, 2);
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
	void operator()(const T& entry, uint64_t* bytes, const int k) {
		memcpy(bytes, entry.meta.data(), entry.meta.size());
	}
};

template<>
struct get_meta<entry_1> {
	void operator()(const entry_1& entry, uint64_t* bytes, const int k);
};

template<typename T>
struct set_meta {
	void operator()(T& entry, const uint64_t* bytes, const size_t num_bytes) {
		memcpy(entry.meta.data(), bytes, num_bytes);
	}
};

template<>
struct set_meta<entry_7> {
	void operator()(entry_7& entry, const uint64_t* bytes, const size_t num_bytes) {
		// no meta data
	}
};

template<typename T>
struct match_t {
	T left;
	T right;
	uintkx_t pos = 0;
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
