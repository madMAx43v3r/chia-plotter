/*
 * entries.h
 *
 *  Created on: May 22, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_ENTRIES_H_
#define INCLUDE_CHIA_ENTRIES_H_

#include <chia/util.hpp>

#include <cstdio>
#include <cstdint>

#ifdef CHIA_K34
	constexpr int KMAX = 34;
	constexpr int PMAX = 35;
	constexpr int KBYTES = 5;
	typedef uint64_t uintkx_t;
	typedef uint128_t uintlp_t;
#else
#define CHIA_K32
	constexpr int KMAX = 32;
	constexpr int PMAX = 32;
	constexpr int KBYTES = 4;
	typedef uint32_t uintkx_t;
	typedef uint64_t uintlp_t;
#endif


template<typename T>
bool write_entry(FILE* file, const T& entry) {
	uint8_t buf[T::disk_size];
	entry.write(buf);
	return fwrite(buf, 1, T::disk_size, file) == T::disk_size;
}

template<typename T>
bool read_entry(FILE* file, T& entry) {
	uint8_t buf[T::disk_size];
	if(fread(buf, 1, T::disk_size, file) != T::disk_size) {
		return false;
	}
	entry.read(buf);
	return true;
}


#endif /* INCLUDE_CHIA_ENTRIES_H_ */
