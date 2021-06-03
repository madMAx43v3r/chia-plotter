// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_CPP_POS_CONSTANTS_HPP_
#define SRC_CPP_POS_CONSTANTS_HPP_

#include <numeric>

namespace chia {

// Unique plot id which will be used as a ChaCha8 key, and determines the PoSpace.
const uint32_t kIdLen = 32;

// Distance between matching entries is stored in the offset
const uint32_t kOffsetSize = 10;

// Max matches a single entry can have, used for hardcoded memory allocation
const uint32_t kMaxMatchesSingleEntry = 30;
const uint32_t kMinBuckets = 16;
const uint32_t kMaxBuckets = 128;

// During backprop and compress, the write pointer is ahead of the read pointer
// Note that the large the offset, the higher these values must be
const uint32_t kReadMinusWrite = 1U << kOffsetSize;
const uint32_t kCachedPositionsSize = kReadMinusWrite * 4;

// Must be set high enough to prevent attacks of fast plotting
const uint32_t kMinPlotSize = 18;

// Set to 50 since k + kExtraBits + k*4 must not exceed 256 (BLAKE3 output size)
const uint32_t kMaxPlotSize = 50;

// The amount of spare space used for sort on disk (multiplied time memory buffer size)
const uint32_t kSpareMultiplier = 5;

// The proportion of memory to allocate to the Sort Manager for reading in buckets and sorting them
// The lower this number, the more memory must be provided by the caller. However, lowering the
// number also allows a higher proportion for writing, which reduces seeks for HDD.
const double kMemSortProportion = 0.75;
const double kMemSortProportionLinePoint = 0.85;

// How many f7s per C1 entry, and how many C1 entries per C2 entry
const uint32_t kCheckpoint1Interval = 10000;
const uint32_t kCheckpoint2Interval = 10000;

// F1 evaluations are done in batches of 2^kBatchSizes
const uint32_t kBatchSizes = 8;

// EPP for the final file, the higher this is, the less variability, and lower delta
// Note: if this is increased, ParkVector size must increase
const uint32_t kEntriesPerPark = 2048;

// To store deltas for EPP entries, the average delta must be less than this number of bits
const double kMaxAverageDeltaTable1 = 5.6;
const double kMaxAverageDelta = 3.5;

// C3 entries contain deltas for f7 values, the max average size is the following
const double kC3BitsPerEntry = 2.4;

// The number of bits in the stub is k minus this value
const uint8_t kStubMinusBits = 3;

// The ANS encoding R values for the 7 final plot tables
// Tweaking the R values might allow lowering of the max average deltas, and reducing final
// plot size
const double kRValues[7] = {4.7, 2.75, 2.75, 2.7, 2.6, 2.45};

// The ANS encoding R value for the C3 checkpoint table
const double kC3R = 1.0;

// Plot format (no compatibility guarantees with other formats). If any of the
// above contants are changed, or file format is changed, the version should
// be incremented.
const std::string kFormatDescription = "v1.0";

struct PlotEntry {
    uint64_t y;
    uint64_t pos;
    uint64_t offset;
    uint128_t left_metadata;   // We only use left_metadata, unless metadata does not
    uint128_t right_metadata;  // fit in 128 bits.
    bool used;                 // Whether the entry was used in the next table of matches
    uint64_t read_posoffset;   // The combined pos and offset that this entry points to
};

}

#endif  // SRC_CPP_POS_CONSTANTS_HPP_
