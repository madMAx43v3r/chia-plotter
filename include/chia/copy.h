/*
 * copy.h
 *
 *  Created on: Jun 8, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_COPY_H_
#define INCLUDE_CHIA_COPY_H_

#include <chia/settings.h>

#include <string>
#include <vector>
#include <stdexcept>

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <errno.h>


inline
uint64_t copy_file(const std::string& src_path, const std::string& dst_path)
{
	FILE* src = fopen(src_path.c_str(), "rb");
	if(!src) {
		throw std::runtime_error("fopen() failed for " + src_path + " (" + std::string(std::strerror(errno)) + ")");
	}
	FILE* dst = fopen(dst_path.c_str(), "wb");
	if(!dst) {
		const auto err = errno;
		fclose(src);
		throw std::runtime_error("fopen() failed for " + dst_path + " (" + std::string(std::strerror(err)) + ")");
	}
	uint64_t total_bytes = 0;
	std::vector<uint8_t> buffer(g_read_chunk_size * 16);
	while(true) {
		const auto num_bytes = fread(buffer.data(), 1, buffer.size(), src);
		if(fwrite(buffer.data(), 1, num_bytes, dst) != num_bytes) {
			const auto err = errno;
			fclose(src);
			fclose(dst);
			throw std::runtime_error("fwrite() failed on " + dst_path + " (" + std::string(std::strerror(err)) + ")");
		}
		total_bytes += num_bytes;
		if(num_bytes < buffer.size()) {
			break;
		}
	}
	fclose(src);

	if(fclose(dst)) {
		throw std::runtime_error("fclose() failed on " + dst_path + " (" + std::string(std::strerror(errno)) + ")");
	}
	return total_bytes;
}

inline
uint64_t final_copy(const std::string& src_path, const std::string& dst_path)
{
	if(src_path == dst_path) {
		return 0;
	}
	const std::string tmp_dst_path = dst_path + ".tmp";
	uint64_t total_bytes = 109521666048ull;
	if(rename(src_path.c_str(), tmp_dst_path.c_str())) {
		// try manual copy
		total_bytes = copy_file(src_path, tmp_dst_path);
	}
	remove(src_path.c_str());
	rename(tmp_dst_path.c_str(), dst_path.c_str());
	return total_bytes;
}


#endif /* INCLUDE_CHIA_COPY_H_ */
