/*
 * phase1.cpp
 *
 *  Created on: May 25, 2021
 *      Author: mad
 */

#include <chia/phase1.h>
#include <chia/ThreadPool.h>

#include "b3/blake3.h"
#include "chacha8.h"


namespace phase1 {

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
	void compute_entry_1_block(const uint64_t index, entry_1* block)
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

/*
 * id = 32 bytes
 */
void compute_f1(const uint8_t* id, int num_threads, Processor<std::vector<entry_1>>* output)
{
	static constexpr size_t M = 4096;
	
	ThreadPool<uint64_t, std::vector<entry_1>> pool(
		[id, M](uint64_t& offset, std::vector<entry_1>& out) {
			out.resize(M * 16);
			F1Calculator F1(id);
			for(size_t i = 0; i < M; ++i) {
				F1.compute_entry_1_block(offset + i, &out[i * 16]);
			}
		}, output, num_threads, "F1");
	
	for(uint64_t k = 0; k < (uint64_t(1) << 28) / M; ++k) {
		pool.take_copy(k);
	}
}



} // phase1
