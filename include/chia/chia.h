/*
 * chia.h
 *
 *  Created on: May 24, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_CHIA_H_
#define INCLUDE_CHIA_CHIA_H_

#include <cstdint>


// Extra bits of output from the f functions. Instead of being a function from k -> k bits,
// it's a function from k -> k + kExtraBits bits. This allows less collisions in matches.
// Refer to the paper for mathematical motivations.
static constexpr uint8_t kExtraBits = 6;

// Convenience variable
static constexpr uint8_t kExtraBitsPow = 1 << kExtraBits;

// B and C groups which constitute a bucket, or BC group. These groups determine how
// elements match with each other. Two elements must be in adjacent buckets to match.
static constexpr uint16_t kB = 119;
static constexpr uint16_t kC = 127;
static constexpr uint16_t kBC = kB * kC;


template <typename Int>
constexpr inline Int cdiv(Int a, int b) { return (a + b - 1) / b; }


#endif /* INCLUDE_CHIA_CHIA_H_ */
