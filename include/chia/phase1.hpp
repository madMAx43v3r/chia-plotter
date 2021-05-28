/*
 * phase1.hpp
 *
 *  Created on: May 26, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_PHASE1_HPP_
#define INCLUDE_CHIA_PHASE1_HPP_

#include <chia/phase1.h>
#include <chia/ThreadPool.h>
#include <chia/bits.hpp>

#include "b3/blake3.h"
#include "chacha8.h"


namespace phase1 {

static uint16_t L_targets[2][kBC][kExtraBitsPow];

static void load_tables()
{
    for (uint8_t parity = 0; parity < 2; parity++) {
        for (uint16_t i = 0; i < kBC; i++) {
            uint16_t indJ = i / kC;
            for (uint16_t m = 0; m < kExtraBitsPow; m++) {
                uint16_t yr =
                    ((indJ + m) % kB) * kC + (((2 * m + parity) * (2 * m + parity) + i) % kC);
                L_targets[parity][i][m] = yr;
            }
        }
    }
}

static void initialize() {
	load_tables();
}

class F1Calculator {
public:
	F1Calculator(const uint8_t* orig_key)
	{
		uint8_t enc_key[32] = {};

		// First byte is 1, the index of this table
		enc_key[0] = 1;
		memcpy(enc_key + 1, orig_key, 31);

		// Setup ChaCha8 context with zero-filled IV
		chacha8_keysetup(&enc_ctx_, enc_key, 256, NULL);
	}

	/*
	 * x = [index * 16 .. index * 16 + 15]
	 * block = entry_1[16]
	 */
	void compute_block(const uint64_t index, entry_1* block)
	{
		uint8_t buf[64];
		chacha8_get_keystream(&enc_ctx_, index, 1, buf);
		
		for(uint64_t i = 0; i < 16; ++i)
		{
			uint64_t y = 0;
			memcpy(&y, buf + i * 4, 4);
			const uint64_t x = index * 16 + i;
			block[i].y = (y << kExtraBits) | (x >> (32 - kExtraBits));
			block[i].x = x;
		}
	}

private:
	chacha8_ctx enc_ctx_ {};
};

// Class to evaluate F2 .. F7.
template<typename T, typename S>
class FxCalculator {
public:
	static constexpr uint8_t k_ = 32;
	
    FxCalculator(int table_index) {
        table_index_ = table_index;
    }

    // Disable copying
    FxCalculator(const FxCalculator&) = delete;

    // Performs one evaluation of the f function.
    void evaluate(const T& L, const T& R, S& entry) const
    {
        Bits C;
        Bits input;
        uint8_t input_bytes[64];
        uint8_t hash_bytes[32];
        uint8_t L_meta[16];
        uint8_t R_meta[16];
        
        size_t L_meta_bytes = 0;
        size_t R_meta_bytes = 0;
        get_meta<T>{}(L, L_meta, &L_meta_bytes);
        get_meta<T>{}(R, R_meta, &R_meta_bytes);
        
        const Bits Y_1(L.y, k_ + kExtraBits);
        const Bits L_c(L_meta, L_meta_bytes, L_meta_bytes * 8);
        const Bits R_c(R_meta, R_meta_bytes, R_meta_bytes * 8);

        if (table_index_ < 4) {
            C = L_c + R_c;
            input = Y_1 + C;
        } else {
            input = Y_1 + L_c + R_c;
        }
        input.ToBytes(input_bytes);

        blake3_hasher hasher;
        blake3_hasher_init(&hasher);
        blake3_hasher_update(&hasher, input_bytes, cdiv(input.GetSize(), 8));
        blake3_hasher_finalize(&hasher, hash_bytes, sizeof(hash_bytes));

        entry.y = Util::EightBytesToInt(hash_bytes) >> (64 - (k_ + (table_index_ < 7 ? kExtraBits : 0)));

        if (table_index_ < 4) {
            // c is already computed
        } else if (table_index_ < 7) {
            uint8_t len = kVectorLens[table_index_ + 1];
            uint8_t start_byte = (k_ + kExtraBits) / 8;
            uint8_t end_bit = k_ + kExtraBits + k_ * len;
            uint8_t end_byte = cdiv(end_bit, 8);

            // TODO: proper support for partial bytes in Bits ctor
            C = Bits(hash_bytes + start_byte, end_byte - start_byte, (end_byte - start_byte) * 8);

            C = C.Slice((k_ + kExtraBits) % 8, end_bit - start_byte * 8);
        }
        uint8_t C_bytes[16];
        C.ToBytes(C_bytes);
        set_meta<S>{}(entry, C_bytes, C.GetSize() / 8);
    }

private:
    int table_index_ = 0;
};

template<typename T>
class FxMatcher {
public:
	struct rmap_item {
		uint16_t pos;
		uint16_t count;
	};
	
    FxMatcher() {
        rmap.resize(kBC);
    }

    // Disable copying
    FxMatcher(const FxMatcher&) = delete;

    // Given two buckets with entries (y values), computes which y values match, and returns a list
    // of the pairs of indices into bucket_L and bucket_R. Indices l and r match iff:
    //   let  yl = bucket_L[l].y,  yr = bucket_R[r].y
    //
    //   For any 0 <= m < kExtraBitsPow:
    //   yl / kBC + 1 = yR / kBC   AND
    //   (yr % kBC) / kC - (yl % kBC) / kC = m   (mod kB)  AND
    //   (yr % kBC) % kC - (yl % kBC) % kC = (2m + (yl/kBC) % 2)^2   (mod kC)
    //
    // Instead of doing the naive algorithm, which is an O(kExtraBitsPow * N^2) comparisons on
    // bucket length, we can store all the R values and lookup each of our 32 candidates to see if
    // any R value matches. This function can be further optimized by removing the inner loop, and
    // being more careful with memory allocation.
    int find_matches_ex(
        const std::vector<T>& bucket_L,
        const std::vector<T>& bucket_R,
        uint16_t* idx_L,
        uint16_t* idx_R)
    {
        if(bucket_L.empty() || bucket_R.empty()) {
        	return 0;
        }
    	const uint16_t parity = (bucket_L[0].y / kBC) % 2;

        for (auto yl : rmap_clean) {
            rmap[yl].count = 0;
        }
        rmap_clean.clear();

        const uint64_t offset = (bucket_R[0].y / kBC) * kBC;
        for (size_t pos_R = 0; pos_R < bucket_R.size(); pos_R++) {
            const uint64_t r_y = bucket_R[pos_R].y - offset;

            if (!rmap[r_y].count) {
                rmap[r_y].pos = pos_R;
            }
            rmap[r_y].count++;
            rmap_clean.push_back(r_y);
        }

        int idx_count = 0;
        const uint64_t offset_y = offset - kBC;
        for (size_t pos_L = 0; pos_L < bucket_L.size(); pos_L++) {
            const uint64_t r = bucket_L[pos_L].y - offset_y;
            for (int i = 0; i < kExtraBitsPow; i++) {
                const uint16_t r_target = L_targets[parity][r][i];
                for (size_t j = 0; j < rmap[r_target].count; j++) {
					idx_L[idx_count] = pos_L;
					idx_R[idx_count] = rmap[r_target].pos + j;
                    idx_count++;
                }
            }
        }
        return idx_count;
    }
    
    int find_matches(	const uint32_t& L_pos_begin,
						const std::vector<T>& bucket_L,
						const std::vector<T>& bucket_R,
						std::vector<match_t<T>>& out)
	{
    	uint16_t idx_L[kBC];
		uint16_t idx_R[kBC];
		const int count = find_matches_ex(bucket_L, bucket_R, idx_L, idx_R);
		
		for(int i = 0; i < count; ++i) {
			match_t<T> match;
			match.left = bucket_L[idx_L[i]];
			match.right = bucket_R[idx_R[i]];
			match.pos = L_pos_begin + idx_L[i];
			match.off = idx_R[i] + (bucket_L.size() - idx_L[i]);
			out.push_back(match);
		}
		return count;
	}

private:
    std::vector<rmap_item> rmap;
    std::vector<uint16_t> rmap_clean;
};

/*
 * id = 32 bytes
 */
template<typename DS>
void compute_f1(const uint8_t* id, int num_threads, DS* T1_sort)
{
	static constexpr size_t M = 4096;	// F1 block size
	
	const auto begin = get_wall_time_micros();
	
	Thread<std::vector<entry_1>> output(
			[T1_sort](std::vector<entry_1>& input) {
				for(const auto& entry : input) {
					T1_sort->add(entry);
//					std::cout << "x=" << entry.x << ", y=" << entry.y << std::endl;
				}
			}, "Disk/add");
	
	ThreadPool<uint64_t, std::vector<entry_1>> pool(
		[id](uint64_t& block, std::vector<entry_1>& out, size_t&) {
			out.resize(M * 16);
			F1Calculator F1(id);
			for(size_t i = 0; i < M; ++i) {
				F1.compute_block(block * M + i, &out[i * 16]);
			}
		}, &output, num_threads, "phase1/F1");
	
	for(uint64_t k = 0; k < (uint64_t(1) << 28) / M; ++k) {
		pool.take_copy(k);
	}
	pool.wait();
	output.wait();
	T1_sort->finish();
	
	std::cout << "Table 1 took " << (get_wall_time_micros() - begin) / 1e6 << " sec" << std::endl;
}

template<typename T, typename S, typename R, typename V, typename DS_L, typename DS_R>
uint64_t compute_matches(	int R_index, int num_threads,
							DS_L* L_sort, DS_R* R_sort,
							Processor<std::vector<R>>* L_tmp_out,
							Processor<std::vector<V>>* R_tmp_out)
{
	std::atomic<uint64_t> num_found {};
	std::array<uint64_t, 2> L_index = {};
	std::array<uint64_t, 2> L_offset = {};
	std::array<std::shared_ptr<std::vector<T>>, 2> L_bucket;
	double avg_bucket_size = 0;
	
	struct match_input_t {
		std::array<uint64_t, 2> L_offset = {};
		std::array<std::shared_ptr<std::vector<T>>, 2> L_bucket;
	};
	
	Thread<std::vector<S>> write_thread(
		[R_sort, R_tmp_out](std::vector<S>& input) {
			if(R_sort) {
				for(const auto& entry : input) {
					R_sort->add(entry);
				}
			}
			if(R_tmp_out) {
				std::vector<V> tmp(input.size());
				for(size_t i = 0; i < tmp.size(); ++i) {
					tmp[i].assign(input[i]);
				}
				R_tmp_out->take(tmp);
			}
		}, "phase1/add");
	
	ThreadPool<std::vector<match_t<T>>, std::vector<S>> eval_pool(
		[R_index](std::vector<match_t<T>>& matches, std::vector<S>& out, size_t&) {
			out.reserve(matches.size());
			FxCalculator<T, S> Fx(R_index);
			for(const auto& match : matches) {
				S entry;
				entry.pos = match.pos;
				entry.off = match.off;
				Fx.evaluate(match.left, match.right, entry);
				out.push_back(entry);
			}
		}, &write_thread, num_threads, "phase1/eval");
	
	ThreadPool<std::vector<match_input_t>, std::vector<match_t<T>>, FxMatcher<T>> match_pool(
		[&num_found](std::vector<match_input_t>& input, std::vector<match_t<T>>& out, FxMatcher<T>& Fx) {
			out.reserve(96 * 1024);
			for(const auto& pair : input) {
				num_found += Fx.find_matches(pair.L_offset[1], *pair.L_bucket[1], *pair.L_bucket[0], out);
			}
		}, &eval_pool, num_threads, "phase1/match");
	
	Thread<std::vector<T>> read_thread(
		[&L_index, &L_offset, &L_bucket, &avg_bucket_size, &match_pool, L_tmp_out](std::vector<T>& input) {
			if(L_tmp_out) {
				std::vector<R> tmp(input.size());
				for(size_t i = 0; i < tmp.size(); ++i) {
					tmp[i].assign(input[i]);
				}
				L_tmp_out->take(tmp);
			}
			std::vector<match_input_t> out;
			out.reserve(1024);
			for(const auto& entry : input) {
				const uint64_t index = entry.y / kBC;
//				std::cout << "x=" << entry.x << ", y=" << entry.y << ", index=" << index << std::endl;
				if(index < L_index[0]) {
					throw std::logic_error("input not sorted");
				}
				if(index > L_index[0]) {
					if(L_index[1] + 1 == L_index[0]) {
						match_input_t pair;
						pair.L_offset = L_offset;
						pair.L_bucket = L_bucket;
						out.push_back(pair);
					}
					L_index[1] = L_index[0];
					L_index[0] = index;
					L_offset[1] = L_offset[0];
					if(auto bucket = L_bucket[0]) {
						L_offset[0] += bucket->size();
						avg_bucket_size = avg_bucket_size * 0.99 + bucket->size() * 0.01;
					}
					L_bucket[1] = L_bucket[0];
					L_bucket[0] = nullptr;
				}
				if(!L_bucket[0]) {
					L_bucket[0] = std::make_shared<std::vector<T>>();
					L_bucket[0]->reserve(avg_bucket_size * 1.2);
				}
				L_bucket[0]->push_back(entry);
			}
//			std::cout << "block size = " << input.size() << ", number of buckets = " << out.size()
//					<< ", avg. bucket size = " << avg_bucket_size << std::endl;
			match_pool.take(out);
		}, "phase1/slice");
	
	L_sort->read(&read_thread);
	
	read_thread.wait();
	match_pool.wait();
	
	if(L_index[1] + 1 == L_index[0]) {
		FxMatcher<T> Fx;
		std::vector<match_t<T>> matches;
		num_found += Fx.find_matches(L_offset[1], *L_bucket[1], *L_bucket[0], matches);
		eval_pool.take(matches);
	}
	eval_pool.wait();
	write_thread.wait();
	
	if(R_sort) {
		R_sort->finish();
	}
	return num_found;
}

template<typename T, typename S, typename R, typename V, typename DS_L, typename DS_R>
uint64_t compute_table(	int R_index, int num_threads,
						DS_L* L_sort, DS_R* R_sort, FILE* L_tmp, FILE* R_tmp = nullptr)
{
	Thread<std::vector<R>> L_write(
		[L_tmp](std::vector<R>& input) {
			for(const auto& entry : input) {
				write_entry(L_tmp, entry);
			}
		}, "phase1/write/L");
	
	Thread<std::vector<V>> R_write(
		[R_tmp](std::vector<V>& input) {
			for(const auto& entry : input) {
				write_entry(R_tmp, entry);
			}
		}, "phase1/write/R");
	
	const auto begin = get_wall_time_micros();
	const auto num_matches =
			phase1::compute_matches<T, S, R, V>(
					R_index, num_threads, L_sort, R_sort,
					L_tmp ? &L_write : nullptr,
					R_tmp ? &R_write : nullptr);
	
	if(L_tmp) {
		L_write.wait();
		fflush(L_tmp);
	}
	if(R_tmp) {
		R_write.wait();
		fflush(R_tmp);
	}
	std::cout << "Table " << R_index << " took " << (get_wall_time_micros() - begin) / 1e6 << " sec"
			<< ", found " << num_matches << " matches" << std::endl;
	return num_matches;
}


} // phase1

#endif /* INCLUDE_CHIA_PHASE1_HPP_ */
