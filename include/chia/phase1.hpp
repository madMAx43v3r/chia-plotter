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
#include <chia/DiskTable.h>
#include <chia/bits.hpp>

#include "blake3.h"
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
	F1Calculator(int k, const uint8_t* orig_key)
		:	k_(k)
	{
		uint8_t enc_key[32] = {};

		// First byte is 1, the index of this table
		enc_key[0] = 1;
		memcpy(enc_key + 1, orig_key, 31);

		// Setup ChaCha8 context with zero-filled IV
		chacha8_keysetup(&enc_ctx_, enc_key, 256, NULL);
	}

	void compute_block(uint64_t first_x, uint64_t num_entries, entry_1* block)
	{
		const uint64_t start = (first_x * k_) / kF1BlockSizeBits;
		// 'end' is one past the last keystream block number to be generated
		const uint64_t end = cdiv((first_x + num_entries) * k_, kF1BlockSizeBits);
		const uint64_t num_blocks = end - start;
		const uint8_t x_shift = k_ - kExtraBits;

		if(num_blocks > 2) {
			throw std::logic_error("num_blocks > 2");
		}
		uint32_t start_bit = (first_x * k_) % kF1BlockSizeBits;

		uint8_t buf[2 * 64];
		chacha8_get_keystream(&this->enc_ctx_, start, num_blocks, buf);

		for(uint64_t x = first_x; x < first_x + num_entries; x++)
		{
			const uint64_t y = Util::SliceInt64FromBytes(buf, start_bit, k_);
			start_bit += k_;

			auto& out = block[x - first_x];
			out.x = x;
			out.y = (y << kExtraBits) | (x >> x_shift);
		}
	}

private:
	int k_ = 0;
	chacha8_ctx enc_ctx_ {};
};

// Class to evaluate F2 .. F7.
template<typename T, typename S>
class FxCalculator {
public:
    FxCalculator(int k, int table_index)
		: k_(k), table_index_(table_index)
	{
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
        uint128_t L_meta;
        uint128_t R_meta;
        
        const int meta_bits = kVectorLens[table_index_] * k_;

        get_meta<T>{}(L, &L_meta);
        get_meta<T>{}(R, &R_meta);
        
        const Bits Y_1(L.y, k_ + kExtraBits);
        const Bits L_c(L_meta, meta_bits);
        const Bits R_c(R_meta, meta_bits);

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
        set_meta<S>{}(entry, Util::SliceInt128FromBytes(C_bytes, 0, C.GetSize()), cdiv(C.GetSize(), 8));
    }

private:
    int k_ = 0;
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
                rmap_clean.push_back(r_y);
            }
            rmap[r_y].count++;
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
    
    int find_matches(	const uint64_t& L_pos_begin,
						const std::vector<T>& bucket_L,
						const std::vector<T>& bucket_R,
						std::vector<match_t<T>>& out)
	{
    	uint16_t idx_L[kBC];
		uint16_t idx_R[kBC];
		const int count = find_matches_ex(bucket_L, bucket_R, idx_L, idx_R);
		
		for(int i = 0; i < count; ++i) {
			const auto pos = L_pos_begin + idx_L[i];
			if(pos < (uint64_t(1) << 32)) {
				match_t<T> match;
				match.left = bucket_L[idx_L[i]];
				match.right = bucket_R[idx_R[i]];
				match.pos = pos;
				match.off = idx_R[i] + (bucket_L.size() - idx_L[i]);
				out.push_back(match);
			}
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
void compute_f1(const uint8_t* id, int k, int num_threads, DS* T1_sort)
{
	static constexpr size_t M = 4096;	// F1 block size
	
	const auto begin = get_wall_time_micros();
	
	typedef typename DS::WriteCache WriteCache;
	
	ThreadPool<std::vector<entry_1>, size_t, std::shared_ptr<WriteCache>> output(
		[T1_sort](std::vector<entry_1>& input, size_t&, std::shared_ptr<WriteCache>& cache) {
			if(!cache) {
				cache = T1_sort->add_cache();
			}
			for(auto& entry : input) {
				cache->add(entry);
			}
		}, nullptr, std::max(num_threads / 2, 1), "phase1/add");
	
	ThreadPool<uint64_t, std::vector<entry_1>> pool(
		[id, k](uint64_t& block, std::vector<entry_1>& out, size_t&) {
			out.resize(M * 16);
			F1Calculator F1(k, id);
			for(size_t i = 0; i < M; ++i) {
				F1.compute_block((block * M + i) * 16, 16, &out[i * 16]);
			}
		}, &output, num_threads, "phase1/F1");
	
	for(uint64_t i = 0; i < (uint64_t(1) << (k - 4)) / M; ++i) {
		pool.take_copy(i);
	}
	pool.close();
	output.close();
	T1_sort->finish();
	
	std::cout << "[P1] Table 1 took " << (get_wall_time_micros() - begin) / 1e6 << " sec" << std::endl;
}

template<typename T, typename S, typename R, typename DS_L, typename DS_R>
uint64_t compute_matches(	int R_index, int k, int num_threads,
							DS_L* L_sort, DS_R* R_sort,
							Processor<std::vector<T>>* L_tmp_out,
							Processor<std::vector<S>>* R_tmp_out)
{
	std::atomic<uint64_t> num_found {};
	std::atomic<uint64_t> num_written {};
	std::array<uint64_t, 2> L_index = {};
	std::array<uint64_t, 2> L_offset = {};
	std::array<std::shared_ptr<std::vector<T>>, 2> L_bucket;
	double avg_bucket_size = 0;
	
	struct match_input_t {
		std::array<uint64_t, 2> L_offset = {};
		std::array<std::shared_ptr<std::vector<T>>, 2> L_bucket;
	};
	
	typedef typename DS_R::WriteCache WriteCache;
	
	ThreadPool<std::vector<S>, size_t, std::shared_ptr<WriteCache>> R_add(
		[R_sort](std::vector<S>& input, size_t&, std::shared_ptr<WriteCache>& cache) {
			if(!cache) {
				cache = R_sort->add_cache();
			}
			for(auto& entry : input) {
				cache->add(entry);
			}
		}, nullptr, std::max(num_threads / 2, 1), "phase1/add");
	
	Processor<std::vector<S>>* R_out = &R_add;
	if(R_tmp_out) {
		R_out = R_tmp_out;
	}
	
	ThreadPool<std::vector<match_t<T>>, std::vector<S>> eval_pool(
		[R_index, k](std::vector<match_t<T>>& matches, std::vector<S>& out, size_t&) {
			out.reserve(matches.size());
			FxCalculator<T, S> Fx(k, R_index);
			for(const auto& match : matches) {
				S entry;
				entry.pos = match.pos;
				entry.off = match.off;
				Fx.evaluate(match.left, match.right, entry);
				out.push_back(entry);
			}
		}, R_out, num_threads, "phase1/eval");
	
	ThreadPool<std::vector<match_input_t>, std::vector<match_t<T>>, FxMatcher<T>> match_pool(
		[&num_found, &num_written]
		 (std::vector<match_input_t>& input, std::vector<match_t<T>>& out, FxMatcher<T>& Fx) {
			out.reserve(64 * 1024);
			for(const auto& pair : input) {
				num_found += Fx.find_matches(pair.L_offset[1], *pair.L_bucket[1], *pair.L_bucket[0], out);
			}
			num_written += out.size();
		}, &eval_pool, num_threads, "phase1/match");
	
	Thread<std::pair<std::vector<T>, size_t>> read_thread(
		[&L_index, &L_offset, &L_bucket, &avg_bucket_size, &match_pool, L_tmp_out]
		 (std::pair<std::vector<T>, size_t>& input) {
			std::vector<match_input_t> out;
			out.reserve(1024);
			for(const auto& entry : input.first) {
				const uint64_t index = entry.y / kBC;
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
			match_pool.take(out);
			if(L_tmp_out) {
				L_tmp_out->take(input.first);
			}
		}, "phase1/slice");
	
	L_sort->read(&read_thread, std::max(num_threads / 2, 2), std::max(num_threads / g_read_thread_divider, 2));
	
	read_thread.close();
	match_pool.close();
	
	if(L_index[1] + 1 == L_index[0]) {
		FxMatcher<T> Fx;
		std::vector<match_t<T>> matches;
		num_found += Fx.find_matches(L_offset[1], *L_bucket[1], *L_bucket[0], matches);
		num_written += matches.size();
		eval_pool.take(matches);
	}
	eval_pool.close();
	R_add.close();
	
	if(R_sort) {
		R_sort->finish();
	}
	if(num_written < num_found) {
//		std::cout << "[P1] Lost " << num_found - num_written
//				<< " matches due to 32-bit overflow." << std::endl;
	}
	return num_written;
}

template<typename T, typename S, typename R, typename DS_L, typename DS_R>
uint64_t compute_table(	int R_index, int k, int num_threads,
						DS_L* L_sort, DS_R* R_sort,
						DiskTable<R>* L_tmp, DiskTable<S>* R_tmp = nullptr)
{
	Thread<std::vector<T>> L_write(
		[L_tmp](std::vector<T>& input) {
			for(const auto& entry : input) {
				R tmp;
				tmp.assign(entry);
				L_tmp->write(tmp);
			}
		}, "phase1/write/L");
	
	Thread<std::vector<S>> R_write(
		[R_tmp](std::vector<S>& input) {
			for(const auto& entry : input) {
				R_tmp->write(entry);
			}
		}, "phase1/write/R");
	
	const auto begin = get_wall_time_micros();
	const auto num_matches =
			phase1::compute_matches<T, S, R>(
					R_index, k, num_threads, L_sort, R_sort,
					L_tmp ? &L_write : nullptr,
					R_tmp ? &R_write : nullptr);
	
	L_write.close();
	R_write.close();
	
	if(L_tmp) {
		L_tmp->close();
	}
	if(R_tmp) {
		R_tmp->close();
	}
	std::cout << "[P1] Table " << R_index << " took " << (get_wall_time_micros() - begin) / 1e6 << " sec"
			<< ", found " << num_matches << " matches" << std::endl;
	return num_matches;
}

inline
void compute(	const input_t& input, output_t& out,
				const int num_threads, const int log_num_buckets,
				const std::string plot_name,
				const std::string tmp_dir,
				const std::string tmp_dir_2)
{
	const auto total_begin = get_wall_time_micros();
	
	initialize();
	
	const int k = input.k;
	const std::string prefix = tmp_dir + plot_name + ".p1.";
	const std::string prefix_2 = tmp_dir_2 + plot_name + ".p1.";
	
	DiskSort1 sort_1(k + kExtraBits, log_num_buckets, prefix_2 + "t1");
	compute_f1(input.id.data(), k, num_threads, &sort_1);
	
	DiskTable<tmp_entry_1> tmp_1(prefix + "table1.tmp");
	DiskSort2 sort_2(k + kExtraBits, log_num_buckets, prefix_2 + "t2");
	compute_table<entry_1, entry_2, tmp_entry_1>(
			2, k, num_threads, &sort_1, &sort_2, &tmp_1);
	
	DiskTable<tmp_entry_x> tmp_2(prefix + "table2.tmp");
	DiskSort3 sort_3(k + kExtraBits, log_num_buckets, prefix_2 + "t3");
	compute_table<entry_2, entry_3, tmp_entry_x>(
			3, k, num_threads, &sort_2, &sort_3, &tmp_2);
	
	DiskTable<tmp_entry_x> tmp_3(prefix + "table3.tmp");
	DiskSort4 sort_4(k + kExtraBits, log_num_buckets, prefix_2 + "t4");
	compute_table<entry_3, entry_4, tmp_entry_x>(
			4, k, num_threads, &sort_3, &sort_4, &tmp_3);
	
	DiskTable<tmp_entry_x> tmp_4(prefix + "table4.tmp");
	DiskSort5 sort_5(k + kExtraBits, log_num_buckets, prefix_2 + "t5");
	compute_table<entry_4, entry_5, tmp_entry_x>(
			5, k, num_threads, &sort_4, &sort_5, &tmp_4);
	
	DiskTable<tmp_entry_x> tmp_5(prefix + "table5.tmp");
	DiskSort6 sort_6(k + kExtraBits, log_num_buckets, prefix_2 + "t6");
	compute_table<entry_5, entry_6, tmp_entry_x>(
			6, k, num_threads, &sort_5, &sort_6, &tmp_5);
	
	DiskTable<tmp_entry_x> tmp_6(prefix + "table6.tmp");
	DiskTable<entry_7> tmp_7(prefix_2 + "table7.tmp");
	compute_table<entry_6, entry_7, tmp_entry_x, DiskSort6, DiskSort7>(
			7, k, num_threads, &sort_6, nullptr, &tmp_6, &tmp_7);
	
	out.params = input;
	out.table[0] = tmp_1.get_info();
	out.table[1] = tmp_2.get_info();
	out.table[2] = tmp_3.get_info();
	out.table[3] = tmp_4.get_info();
	out.table[4] = tmp_5.get_info();
	out.table[5] = tmp_6.get_info();
	out.table[6] = tmp_7.get_info();
	
	std::cout << "Phase 1 took " << (get_wall_time_micros() - total_begin) / 1e6 << " sec" << std::endl;
}


} // phase1

#endif /* INCLUDE_CHIA_PHASE1_HPP_ */
