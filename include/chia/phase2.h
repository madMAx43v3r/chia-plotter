/*
 * phase2.h
 *
 *  Created on: May 29, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_PHASE2_H_
#define INCLUDE_CHIA_PHASE2_H_

#include <chia/chia.h>
#include <chia/entries.h>

#include <array>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstring>


namespace phase2 {

struct ouput_t {
	std::vector<bool> bitfield_1;
};

struct entry_t {
	uint32_t key;
	uint32_t pos;
	uint16_t off;		// 10 bit
	
	static constexpr size_t disk_size = 10;
	
	size_t read(const uint8_t* buf) {
		memcpy(&key, buf, 4);
		memcpy(&pos, buf + 4, 4);
		memcpy(&off, buf + 8, 2);
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &key, 4);
		memcpy(buf + 4, &pos, 4);
		memcpy(buf + 8, &off, 2);
		return disk_size;
	}
};

template<typename T>
struct get_pos {
	uint64_t operator()(const T& entry) {
		return entry.pos;
	}
};



} // phase2

#endif /* INCLUDE_CHIA_PHASE2_H_ */
