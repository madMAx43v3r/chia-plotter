/*
 * phase3.h
 *
 *  Created on: May 30, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_PHASE3_H_
#define INCLUDE_CHIA_PHASE3_H_

#include <chia/phase2.h>


namespace phase3 {

struct entry_kpp {
	uint32_t pos[2];	// 2x 32-bit position
	uint32_t key;		// 32-bit (sort_key)
};

struct entry_lp {
	uint64_t point;		// 63-bit (line_point)
	uint32_t key;		// 32-bit (sort_key)
	
	static constexpr size_t disk_size = 12;
	
	size_t read(const uint8_t* buf) {
		memcpy(&point, buf, 8);
		memcpy(&key, buf + 8, 4);
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &point, 8);
		memcpy(buf + 8, &key, 4);
		return disk_size;
	}
};

struct entry_np {
	uint32_t key;		// 32-bit (sort_key)
	uint32_t pos;		// 32-bit (new_pos)
	
	static constexpr size_t disk_size = 8;
	
	size_t read(const uint8_t* buf) {
		memcpy(&key, buf, 4);
		memcpy(&pos, buf + 4, 4);
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &key, 4);
		memcpy(buf + 4, &pos, 4);
		return disk_size;
	}
};

template<typename T>
struct get_new_pos {
	uint32_t operator()(const T& entry) {
		return entry.pos;
	}
};

template<>
struct get_new_pos<phase2::entry_1> {
	uint32_t operator()(const phase2::entry_1& entry) {
		return entry.x;
	}
};

template<typename T>
struct get_sort_key {
	uint32_t operator()(const T& entry) {
		return entry.key;
	}
};

template<>
struct get_sort_key<phase2::entry_7> {
	uint32_t operator()(const phase2::entry_7& entry) {
		return entry.y;
	}
};

template<typename T>
struct get_line_point {
	uint64_t operator()(const T& entry) {
		return entry.point;
	}
};

typedef DiskSort<entry_lp, get_line_point<entry_lp>> DiskSortLP;
typedef DiskSort<entry_np, get_sort_key<entry_np>> DiskSortNP;

struct output_t {
	int header_size = 0;
	uint64_t num_written_7 = 0;
	uint64_t final_pointer_7 = 0;
	phase1::input_t params;
	std::string plot_file_name;
	std::shared_ptr<DiskSortNP> sort_7;
};


} // phase3

#endif /* INCLUDE_CHIA_PHASE3_H_ */
