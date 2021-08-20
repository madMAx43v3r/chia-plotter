/*
 * chia.h
 *
 *  Created on: May 24, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_CHIA_H_
#define INCLUDE_CHIA_CHIA_H_

#include <chrono>
#include <cstdint>
#include <string>

#include <chia/settings.h>


// Unique plot id which will be used as a ChaCha8 key, and determines the PoSpace.
const uint32_t kIdLen = 32;

// Extra bits of output from the f functions. Instead of being a function from k -> k bits,
// it's a function from k -> k + kExtraBits bits. This allows less collisions in matches.
// Refer to the paper for mathematical motivations.
static constexpr uint8_t kExtraBits = 6;

// Convenience variable
static constexpr uint8_t kExtraBitsPow = 1 << kExtraBits;

// Distance between matching entries is stored in the offset
static constexpr uint32_t kOffsetSize = 10;

// ChaCha8 block size
const uint16_t kF1BlockSizeBits = 512;

// B and C groups which constitute a bucket, or BC group. These groups determine how
// elements match with each other. Two elements must be in adjacent buckets to match.
static constexpr uint16_t kB = 119;
static constexpr uint16_t kC = 127;
static constexpr uint16_t kBC = kB * kC;

// This (times k) is the length of the metadata that must be kept for each entry. For example,
// for a table 4 entry, we must keep 4k additional bits for each entry, which is used to
// compute f5.
static const uint8_t kVectorLens[] = {0, 0, 1, 2, 4, 4, 3, 2};

// The number of bits in the stub is k minus this value
static constexpr uint8_t kStubMinusBits = 3;

// EPP for the final file, the higher this is, the less variability, and lower delta
// Note: if this is increased, ParkVector size must increase
static constexpr uint32_t kEntriesPerPark = 2048;

// To store deltas for EPP entries, the average delta must be less than this number of bits
static constexpr double kMaxAverageDeltaTable1 = 5.6;
static constexpr double kMaxAverageDelta = 3.5;

// How many f7s per C1 entry, and how many C1 entries per C2 entry
static constexpr uint32_t kCheckpoint1Interval = 10000;
static constexpr uint32_t kCheckpoint2Interval = 10000;

// C3 entries contain deltas for f7 values, the max average size is the following
static constexpr double kC3BitsPerEntry = 2.4;

// The ANS encoding R values for the 7 final plot tables
// Tweaking the R values might allow lowering of the max average deltas, and reducing final
// plot size
static const double kRValues[7] = {4.7, 2.75, 2.75, 2.7, 2.6, 2.45};

// The ANS encoding R value for the C3 checkpoint table
static constexpr double kC3R = 1.0;

// Plot format (no compatibility guarantees with other formats). If any of the
// above contants are changed, or file format is changed, the version should
// be incremented.
static const std::string kFormatDescription = "v1.0";


struct table_t {
	std::string file_name;
	size_t num_entries = 0;
};


#endif /* INCLUDE_CHIA_CHIA_H_ */
