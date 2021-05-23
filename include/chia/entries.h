/*
 * entries.h
 *
 *  Created on: May 22, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_ENTRIES_H_
#define INCLUDE_CHIA_ENTRIES_H_

#include <array>
#include <stdint.h>

// Extra bits of output from the f functions. Instead of being a function from k -> k bits,
// it's a function from k -> k + kExtraBits bits. This allows less collisions in matches.
// Refer to the paper for mathematical motivations.
const uint8_t kExtraBits = 6;

// Convenience variable
const uint8_t kExtraBitsPow = 1 << kExtraBits;

// B and C groups which constitute a bucket, or BC group. These groups determine how
// elements match with each other. Two elements must be in adjacent buckets to match.
const uint16_t kB = 119;
const uint16_t kC = 127;
const uint16_t kBC = kB * kC;


namespace phase1 {

struct entry_1 {
	uint64_t f : 38;
	uint32_t x;
};

struct entry_t {
	uint64_t f : 38;
	uint16_t off : 10;
	uint32_t pos;
};

template<int N>
struct entry_tx : entry_t {
	std::array<uint32_t, N> C;
};

typedef entry_tx<1> entry_2;
typedef entry_tx<2> entry_3;
typedef entry_tx<4> entry_4;
typedef entry_tx<4> entry_5;
typedef entry_tx<3> entry_6;
typedef entry_tx<2> entry_7;

} // phase1


namespace phase2 {

struct entry_t {
	uint32_t key;
	uint32_t pos;
	uint16_t off : 10;
};


} // phase2


#endif /* INCLUDE_CHIA_ENTRIES_H_ */
