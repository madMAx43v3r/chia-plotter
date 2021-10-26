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

#ifndef SRC_CPP_BITS_HPP_
#define SRC_CPP_BITS_HPP_

#include <algorithm>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <chia/util.hpp>
#include <chia/exceptions.hpp>

// 64 * 2^16. 2^17 values, each value can store 64 bits.
#define kMaxSizeBits 8388608

// A stack vector of length 5, having the functions of std::vector needed for Bits.
struct SmallVector {
    typedef uint16_t size_type;

    SmallVector() noexcept { count_ = 0; }

    uint64_t& operator[](const uint16_t index) { return v_[index]; }

    uint64_t operator[](const uint16_t index) const { return v_[index]; }

    void push_back(uint64_t value) { v_[count_++] = value; }

    SmallVector& operator=(const SmallVector& other)
    {
        count_ = other.count_;
        for (size_type i = 0; i < other.count_; i++) v_[i] = other.v_[i];
        return (*this);
    }

    size_type size() const noexcept { return count_; }

    void resize(const size_type n) { count_ = n; }

private:
    uint64_t v_[10];
    size_type count_;
};

// A stack vector of length 1024, having the functions of std::vector needed for Bits.
// The max number of Bits that can be stored is 1024 * 64
struct ParkVector {
    typedef uint32_t size_type;

    ParkVector() noexcept { count_ = 0; }

    uint64_t& operator[](const uint32_t index) { return v_[index]; }

    uint64_t operator[](const uint32_t index) const { return v_[index]; }

    void push_back(uint64_t value) { v_[count_++] = value; }

    ParkVector& operator=(const ParkVector& other)
    {
        count_ = other.count_;
        for (size_type i = 0; i < other.count_; i++) v_[i] = other.v_[i];
        return (*this);
    }

    size_type size() const noexcept { return count_; }

private:
    uint64_t v_[2048];
    size_type count_;
};

/*
 * This class represents an array of bits. These are stored in an
 * array of integers, allowing for efficient bit manipulations. The Bits class provides
 * utilities to easily work with Bits, adding and slicing them, etc.
 * The class is a generic one, allowing any type of an array, as long as providing std::vector
 * methods. We currently use SmallVector (stack-array of length 10), ParkVector (stack-array of
 * length 2048) and std::vector. Conversion between two BitsGeneric<T> classes of different types
 * can be done by using += operator, or converting to bytes the first class, then using the bytes
 * constructor of the second class (should be slower). NOTE: CalculateBucket only accepts a
 * BitsGeneric<SmallVector>, so in order to use that, you have to firstly convert your
 * BitsGeneric<T> object into a BitsGeneric<SmallVector>.
 */

template <class T>
class BitsGeneric {
public:
    template <class>
    friend class BitsGeneric;

    BitsGeneric<T>() noexcept { this->last_size_ = 0; }

    // Converts from unit64_t to Bits. If the number of bits of value is smaller than size, adds 0
    // bits at the beginning. i.e. Bits(5, 10) = 0000000101
    BitsGeneric<T>(uint128_t value, uint32_t size)
    {
        if (size > 64) {
            // std::cout << "SPLITTING BitsGeneric" << std::endl;
            InitBitsGeneric(value >> 64, size - 64);
            AppendValue((uint64_t)value, 64);
        } else {
            InitBitsGeneric((uint64_t)value, size);
        }
    }

    // Converts from unit64_t to Bits. If the number of bits of value is smaller than size, adds 0
    // bits at the beginning. i.e. Bits(5, 10) = 0000000101
    void InitBitsGeneric(uint64_t value, uint32_t size)
    {
        this->last_size_ = 0;
        if (size > 64) {
            // Get number of extra 0s added at the beginning.
            uint32_t zeros = size - Util::GetSizeBits(value);
            // Add a full group of 0s (length 64)
            while (zeros > 64) {
                AppendValue(0, 64);
                zeros -= 64;
            }
            // Add the incomplete group of 0s and then the value.
            AppendValue(0, zeros);
            AppendValue(value, Util::GetSizeBits(value));
        } else {
            /* 'value' must be under 'size' bits. */
            assert(size == 64 || value == (value & ((1ULL << size) - 1)));
            values_.push_back(value);
            this->last_size_ = size;
        }
    }

    // Copy the content of another Bits object. If the size of the other Bits object is smaller
    // than 'size', adds 0 bits at the beginning.
    BitsGeneric<T>(const BitsGeneric<T>& other, uint32_t size)
    {
        uint32_t total_size = other.GetSize();
        this->last_size_ = 0;
        assert(size >= total_size);
        // Add the extra 0 bits at the beginning.
        uint32_t extra_space = size - total_size;
        while (extra_space >= 64) {
            AppendValue(0, 64);
            extra_space -= 64;
        }
        if (extra_space > 0)
            AppendValue(0, extra_space);
        // Copy the Bits object element by element, and append it to the current Bits object.
        if (other.values_.size() > 0) {
            for (uint32_t i = 0; i < other.values_.size() - 1; i++)
                AppendValue(other.values_[i], 64);
            AppendValue(other.values_[other.values_.size() - 1], other.last_size_);
        }
    }

    // Converts bytes to bits.
    BitsGeneric<T>(const uint8_t* big_endian_bytes, uint32_t num_bytes, uint32_t size_bits)
    {
        this->last_size_ = 0;
        int32_t extra_space = int32_t(size_bits) - num_bytes * 8;
        while (extra_space >= 64) {
            AppendValue(0, 64);
            extra_space -= 64;
        }
        if (extra_space > 0) {
            AppendValue(0, extra_space);
        }
        for (uint32_t i = 0; i < num_bytes; i += sizeof(uint64_t) / sizeof(uint8_t)) {
            uint64_t val = 0;
            uint8_t bucket_size = 0;
            // Compress bytes together into uint64_t, either until we have 64 bits, or until we run
            // out of bytes in big_endian_bytes.
            for (uint32_t j = i; j < i + sizeof(uint64_t) / sizeof(uint8_t) && j < num_bytes; j++) {
                val = (val << 8) + big_endian_bytes[j];
                bucket_size += 8;
            }
            AppendValue(val, bucket_size);
        }
    }

    BitsGeneric<T>(const BitsGeneric<T>& other) noexcept
        : values_(other.values_), last_size_(other.last_size_)
    {
    }

    BitsGeneric<T>& operator=(const BitsGeneric<T>& other)
    {
        values_ = other.values_;
        last_size_ = other.last_size_;
        return *this;
    }

    // Concatenates two Bits objects together.
    BitsGeneric<T> operator+(const BitsGeneric<T>& b) const
    {
        BitsGeneric<T> result = *this;

        if (b.values_.size() > 0) {
            for (typename T::size_type i = 0; i < b.values_.size() - 1; i++)
                result.AppendValue(b.values_[i], 64);
            result.AppendValue(b.values_[b.values_.size() - 1], b.last_size_);
        }
        return result;
    }

    // Appends one Bits object at the end of the first one.
    template <class T2>
    BitsGeneric<T>& operator+=(const BitsGeneric<T2>& b)
    {
        if (b.values_.size() > 0) {
            for (typename T2::size_type i = 0; i < b.values_.size() - 1; i++)
                this->AppendValue(b.values_[i], 64);
            this->AppendValue(b.values_[b.values_.size() - 1], b.last_size_);
        }
        return *this;
    }

    BitsGeneric<T> Slice(uint32_t start_index) const { return Slice(start_index, GetSize()); }

    // Slices the bits from [start_index, end_index)
    BitsGeneric<T> Slice(uint32_t start_index, uint32_t end_index) const
    {
        if (end_index > GetSize()) {
            end_index = GetSize();
        }

        if (end_index == start_index)
            return BitsGeneric<T>();
        assert(end_index > start_index);
        uint32_t start_bucket = start_index / 64;
        uint32_t end_bucket = end_index / 64;
        if (start_bucket == end_bucket) {
            // Positions inside the bucket.
            start_index = start_index % 64;
            end_index = end_index % 64;
            uint8_t bucket_size =
                ((int)start_bucket == (int)(values_.size() - 1)) ? last_size_ : 64;
            uint64_t val = values_[start_bucket];
            // Cut the prefix [0, start_index)
            if (start_index != 0)
                val = val & ((static_cast<uint64_t>(1) << (bucket_size - start_index)) - 1);
            // Cut the suffix after end_index
            val = val >> (bucket_size - end_index);
            return BitsGeneric<T>(val, end_index - start_index);
        } else {
            BitsGeneric<T> result;
            uint64_t prefix, suffix;
            // Get the prefix from the last bucket.
            SplitNumberByPrefix(values_[start_bucket], 64, start_index % 64, &prefix, &suffix);
            result.AppendValue(suffix, 64 - start_index % 64);
            // Append all the in between buckets
            for (uint32_t i = start_bucket + 1; i < end_bucket; i++)
                result.AppendValue(values_[i], 64);
            if (end_index % 64) {
                uint8_t bucket_size =
                    ((int)end_bucket == (int)(values_.size() - 1)) ? last_size_ : 64;
                // Get the suffix from the last bucket.
                SplitNumberByPrefix(
                    values_[end_bucket], bucket_size, end_index % 64, &prefix, &suffix);
                result.AppendValue(prefix, end_index % 64);
            }
            return result;
        }
    }

    // Same as 'Slice', but result fits into an uint64_t. Used for memory optimization.
    uint64_t SliceBitsToInt(uint32_t start_index, uint32_t end_index) const
    {
        /*if (end_index > GetSize()) {
            end_index = GetSize();
        }
        if (start_index < 0) {
            start_index = 0;
        } */
        if ((start_index >> 6) == (end_index >> 6)) {
            uint64_t res = values_[start_index >> 6];
            if ((start_index >> 6) == (uint32_t)values_.size() - 1)
                res = res >> (last_size_ - (end_index & 63));
            else
                res = res >> (64 - (end_index & 63));
            res = res & (((uint64_t)1 << ((end_index & 63) - (start_index & 63))) - 1);
            return res;
        } else {
            assert((start_index >> 6) + 1 == (end_index >> 6));
            uint64_t prefix, suffix;
            SplitNumberByPrefix(
                values_[(start_index >> 6)], 64, start_index & 63, &prefix, &suffix);
            uint64_t result = suffix;
            if (end_index % 64) {
                uint8_t bucket_size =
                    ((end_index >> 6) == (uint32_t)values_.size() - 1) ? last_size_ : 64;
                SplitNumberByPrefix(
                    values_[(end_index >> 6)], bucket_size, end_index & 63, &prefix, &suffix);
                result = (result << (end_index & 63)) + prefix;
            }
            return result;
        }
    }

    void ToBytes(uint8_t buffer[]) const
    {
        int i;
        uint8_t tmp[8];

        // Return if nothing to work on
        if (!values_.size())
            return;

        for (i = 0; i < (int)values_.size() - 1; i++) {
            Util::IntToEightBytes(buffer + i * 8, values_[i]);
        }

        Util::IntToEightBytes(tmp, values_[i] << (64 - last_size_));
        memcpy(buffer + i * 8, tmp, cdiv(last_size_, 8));
    }

    std::string ToString() const
    {
        std::string str = "";
        for (typename T::size_type i = 0; i < values_.size(); i++) {
            uint64_t val = values_[i];
            typename T::size_type size = (i == values_.size() - 1) ? last_size_ : 64;
            std::string str_bucket = "";
            for (typename T::size_type i = 0; i < size; i++) {
                if (val % 2)
                    str_bucket = "1" + str_bucket;
                else
                    str_bucket = "0" + str_bucket;
                val /= 2;
            }
            str += str_bucket;
        }
        return str;
    }

    uint64_t GetValue() const
    {
        if (values_.size() != 1) {
            std::cout << "Number of 64 bit values is: " << values_.size() << std::endl;
            std::cout << "Size of bits is: " << GetSize() << std::endl;
            throw InvalidStateException(
                "Number doesn't fit into a 64-bit type. " + std::to_string(GetSize()));
        }
        return values_[0];
    }

    uint32_t GetSize() const
    {
        if (values_.size() == 0)
            return 0;
        // Full buckets contain each 64 bits, last one contains only 'last_size_' bits.
        return ((uint32_t)values_.size() - 1) * 64 + last_size_;
    }

    void AppendValue(uint128_t value, uint8_t length)
    {
        if (length > 64) {
            std::cout << "SPLITTING AppendValue" << std::endl;
            DoAppendValue(value >> 64, length - 64);
            DoAppendValue((uint64_t)value, 64);
        } else {
            DoAppendValue((uint64_t)value, length);
        }
    }

    void DoAppendValue(uint64_t value, uint8_t length)
    {
        // The last bucket is full or no bucket yet, create a new one.
        if (values_.size() == 0 || last_size_ == 64) {
            values_.push_back(value);
            last_size_ = length;
        } else {
            uint8_t free_bits = 64 - last_size_;
            if (last_size_ == 0 && length == 64) {
                // Special case for OSX -O3, as per -fsanitize=undefined
                // runtime error: shift exponent 64 is too large for 64-bit type 'uint64_t' (aka
                // 'unsigned long long')
                values_[values_.size() - 1] = value;
                last_size_ = length;
            } else if (length <= free_bits) {
                // If the value fits into the last bucket, append it all there.
                values_[values_.size() - 1] = (values_[values_.size() - 1] << length) + value;
                last_size_ += length;
            } else {
                // Otherwise, append the prefix into the last bucket, and create a new bucket for
                // the suffix.
                uint64_t prefix, suffix;
                SplitNumberByPrefix(value, length, free_bits, &prefix, &suffix);
                values_[values_.size() - 1] = (values_[values_.size() - 1] << free_bits) + prefix;
                values_.push_back(suffix);
                last_size_ = length - free_bits;
            }
        }
    }

    template <class X>
    friend std::ostream& operator<<(std::ostream&, const BitsGeneric<X>&);
    template <class X>
    friend bool operator==(const BitsGeneric<X>& lhs, const BitsGeneric<X>& rhs);
    template <class X>
    friend bool operator<(const BitsGeneric<X>& lhs, const BitsGeneric<X>& rhs);
    template <class X>
    friend bool operator>(const BitsGeneric<X>& lhs, const BitsGeneric<X>& rhs);
    template <class X>
    friend BitsGeneric<X> operator<<(BitsGeneric<X> lhs, uint32_t shift_amount);
    template <class X>
    friend BitsGeneric<X> operator>>(BitsGeneric<X> lhs, uint32_t shift_amount);

private:
    static void SplitNumberByPrefix(
        uint64_t number,
        uint8_t num_bits,
        uint8_t prefix_size,
        uint64_t* prefix,
        uint64_t* suffix)
    {
        assert(num_bits >= prefix_size);
        if (prefix_size == 0) {
            *prefix = 0;
            *suffix = number;
            return;
        }
        uint8_t suffix_size = num_bits - prefix_size;
        uint64_t mask = (static_cast<uint64_t>(1)) << suffix_size;
        mask--;
        *suffix = number & mask;
        *prefix = number >> suffix_size;
    }

    T values_;
    uint8_t last_size_;
};

template <class T>
std::ostream& operator<<(std::ostream& strm, BitsGeneric<T> const& v)
{
    strm << "b" << v.ToString();
    return strm;
}

template <class T>
bool operator==(const BitsGeneric<T>& lhs, const BitsGeneric<T>& rhs)
{
    if (lhs.GetSize() != rhs.GetSize()) {
        return false;
    }
    for (uint32_t i = 0; i < lhs.values_.size(); i++) {
        if (lhs.values_[i] != rhs.values_[i]) {
            return false;
        }
    }
    return true;
}

template <class T>
bool operator<(const BitsGeneric<T>& lhs, const BitsGeneric<T>& rhs)
{
    if (lhs.GetSize() != rhs.GetSize())
        throw InvalidStateException("Different sizes!");
    for (uint32_t i = 0; i < lhs.values_.size(); i++) {
        if (lhs.values_[i] < rhs.values_[i])
            return true;
        if (lhs.values_[i] > rhs.values_[i])
            return false;
    }
    return false;
}

template <class T>
bool operator>(const BitsGeneric<T>& lhs, const BitsGeneric<T>& rhs)
{
    if (lhs.GetSize() != rhs.GetSize())
        throw InvalidStateException("Different sizes!");
    for (uint32_t i = 0; i < lhs.values_.size(); i++) {
        if (lhs.values_[i] > rhs.values_[i])
            return true;
        if (lhs.values_[i] < rhs.values_[i])
            return false;
    }
    return false;
}

template <class T>
BitsGeneric<T> operator<<(BitsGeneric<T> lhs, uint32_t shift_amount)
{
    if (lhs.GetSize() == 0) {
        return BitsGeneric<T>();
    }
    BitsGeneric<T> result;
    // Shifts are cyclic, shifting by the number of bits gives the same number.
    int num_blocks_shift = static_cast<int>(shift_amount / 64);
    uint32_t shift_remainder = shift_amount % 64;
    for (uint32_t i = 0; i < lhs.values_.size(); i++) {
        uint64_t new_value = 0;
        if (i + num_blocks_shift < lhs.values_.size()) {
            new_value += (lhs.values_[i + num_blocks_shift] << shift_remainder);
        }
        if (i + num_blocks_shift + 1 < lhs.values_.size()) {
            new_value += (lhs.values_[i + num_blocks_shift + 1] >> (64 - shift_remainder));
        }
        uint8_t new_length;
        if (i == (uint32_t)lhs.values_.size() - 1) {
            new_length = lhs.last_size_;
        } else {
            new_length = 64;
        }
        result.AppendValue(new_value, new_length);
    }
    return result;
}

template <class T>
BitsGeneric<T> operator>>(BitsGeneric<T> lhs, uint32_t shift_amount)
{
    if (lhs.GetSize() == 0) {
        return BitsGeneric<T>();
    }
    BitsGeneric<T> result;

    int num_blocks_shift = static_cast<int>(shift_amount / 64);
    uint32_t shift_remainder = shift_amount % 64;

    for (int i = 0; i < lhs.values_.size(); i++) {
        uint64_t new_value = 0;
        if (i - num_blocks_shift >= 0) {
            new_value += (lhs.values_[i - num_blocks_shift] >> shift_remainder);
        }
        if (i - num_blocks_shift - 1 >= 0) {
            new_value += (lhs.values_[i - num_blocks_shift - 1] << (64 - shift_remainder));
        }
        uint8_t new_length;
        if (i == lhs.values_.size() - 1) {
            new_length = lhs.last_size_;
        } else {
            new_length = 64;
        }
        result.AppendValue(new_value, new_length);
    }
    return result;
}

typedef std::vector<uint64_t> LargeVector;
using Bits = BitsGeneric<SmallVector>;
using ParkBits = BitsGeneric<ParkVector>;
using LargeBits = BitsGeneric<LargeVector>;


inline
int write_bits(uint64_t* dst, const uint64_t value, const int bit_offset, const int num_bits)
{
	assert(num_bits <= 64);
	const int free_bits = 64 - (bit_offset % 64);
	if(free_bits >= num_bits) {
		dst[bit_offset / 64]     |= bswap_64(value << (free_bits - num_bits));
	} else {
		const int suffix_size = num_bits - free_bits;
		const uint64_t suffix = value & ((uint64_t(1) << suffix_size) - 1);
		dst[bit_offset / 64]     |= bswap_64(value >> suffix_size);			// prefix (high bits)
		dst[bit_offset / 64 + 1] |= bswap_64(suffix << (64 - suffix_size));	// suffix (low bits)
	}
	return bit_offset + num_bits;
}

inline
int append_bits(uint64_t* dst, const uint64_t* src, const int bit_offset, const int num_bits)
{
	int i = 0;
	int offset = bit_offset;
	int num_left = num_bits;
	while(num_left > 0) {
		int bits = 64;
		uint64_t value = bswap_64(src[i]);
		if(num_left < 64) {
			bits = num_left;
			value >>= (64 - num_left);
		}
		offset = write_bits(dst, value, offset, bits);
		num_left -= bits;
		i++;
	}
	return offset;
}

inline
int slice_bits(uint64_t* dst, const uint64_t* src, const int bit_offset, const int num_bits)
{
	int count = 0;
	int offset = bit_offset;
	int num_left = num_bits;
	while(num_left > 0) {
		const int shift = offset % 64;
		const int bits = std::min(num_left, 64 - shift);
		uint64_t value = bswap_64(src[offset / 64]) << shift;
		if(bits < 64) {
			value >>= (64 - bits);
		}
		count = write_bits(dst, value, count, bits);
		offset += bits;
		num_left -= bits;
	}
	return count;
}

#endif  // SRC_CPP_BITS_HPP_
