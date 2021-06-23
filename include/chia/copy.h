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

#ifdef __linux__
	#include <filesystem>
	namespace fs = std::filesystem;
#endif

inline
uint64_t copy_file(const std::string& src_path, const std::string& dst_path, const std::string& dst2_path)
{
	#ifdef __linux__
		const uint64_t plotsize = 109000000000 // rough plot size placeholder, later pass real size through
		fs::space_info tmp = fs::space(dst_path);
		if(tmp.available < plotsize) {
			fs::space_info tmp2 = fs::space(dst_path2);
			if(tmp2.available < plotsize) {
				throw std::runtime_error("All destinations have run out of disk space");
			}
			dst_path = dst2_path;
			throw std::runtime_error("Destination does not have enough available disk space left, switching to alternate destination");
		}
	#endif
	FILE* src = fopen(src_path.c_str(), "rb");
	if(!src) {
		throw std::runtime_error("fopen() failed");
	}
	FILE* dst = fopen(dst_path.c_str(), "wb");
	if(!dst) {
		throw std::runtime_error("fopen() failed");
	}
	uint64_t total_bytes = 0;
	std::vector<uint8_t> buffer(g_read_chunk_size * 16);
	while(true) {
		const auto num_bytes = fread(buffer.data(), 1, buffer.size(), src);
		if(fwrite(buffer.data(), 1, num_bytes, dst) != num_bytes) {
			throw std::runtime_error("fwrite() failed");
		}
		total_bytes += num_bytes;
		if(num_bytes < buffer.size()) {
			break;
		}
	}
	if(fclose(dst)) {
		throw std::runtime_error("fclose() failed");
	}
	fclose(src);
	return total_bytes;
}

inline
uint64_t final_copy(const std::string& src_path, const std::string& dst_path)
{
	if(src_path == dst_path) {
		return 0;
	}
	const std::string tmp_dst_path = dst_path + ".tmp";
	uint64_t total_bytes = 0;
	if(rename(src_path.c_str(), tmp_dst_path.c_str())) {
		// try manual copy
		total_bytes = copy_file(src_path, tmp_dst_path);
	}
	remove(src_path.c_str());
	rename(tmp_dst_path.c_str(), dst_path.c_str());
	return total_bytes;
}


#endif /* INCLUDE_CHIA_COPY_H_ */
