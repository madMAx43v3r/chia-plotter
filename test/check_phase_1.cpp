/*
 * check_phase_1.cpp
 *
 *  Created on: Jun 3, 2021
 *      Author: mad
 */

#include <chia/phase1.h>

#include "chia_ref/verifier.hpp"

using namespace phase1;

std::array<table_t, 7> table;
std::array<FILE*, 7> file;

void gather_x(int depth, uint64_t pos, uint16_t off, std::vector<uint32_t>& out)
{
	FILE* f = file[depth];
	if(depth == 0) {
		tmp_entry_1 entry = {};
		fseek_set(f, pos * tmp_entry_1::disk_size);
		read_entry(f, entry);
//		std::cout << "[" << depth + 1 << "] x=" << entry.x << std::endl;
		out.push_back(entry.x);
		
		fseek_set(f, (pos + off) * tmp_entry_1::disk_size);
		read_entry(f, entry);
//		std::cout << "[" << depth + 1 << "] x=" << entry.x << std::endl;
		out.push_back(entry.x);
	} else {
		tmp_entry_x entry = {};
		fseek_set(f, pos * tmp_entry_x::disk_size);
		read_entry(f, entry);
//		std::cout << "[" << depth + 1 << "] pos=" << entry.pos << ", off=" << entry.off << std::endl;
		gather_x(depth - 1, entry.pos, entry.off, out);
		
		fseek_set(f, (pos + off) * tmp_entry_x::disk_size);
		read_entry(f, entry);
//		std::cout << "[" << depth + 1 << "] pos=" << entry.pos << ", off=" << entry.off << std::endl;
		gather_x(depth - 1, entry.pos, entry.off, out);
	}
}

uint32_t gather_7(uint64_t pos, std::vector<uint32_t>& out)
{
	entry_7 entry = {};
	fseek_set(file[6], pos * entry_7::disk_size);
	read_entry(file[6], entry);
//	std::cout << "[7] y=" << entry.y << ", pos=" << entry.pos << ", off=" << entry.off << std::endl;
	gather_x(5, entry.pos, entry.off, out);
	return entry.y;
}


int main()
{
	uint8_t id[32] = {};
	for(size_t i = 0; i < sizeof(id); ++i) {
		id[i] = i + 1;
	}
	
	for(size_t i = 0; i < table.size(); ++i)
	{
		const std::string file_name = "test.p1.table" + std::to_string(i + 1) + ".tmp";
		size_t size = 0;
		if(i == 0) {
			size = phase1::tmp_entry_1::disk_size;
		} else if(i < 6) {
			size = phase1::tmp_entry_x::disk_size;
		} else if(i == 6) {
			size = phase1::entry_7::disk_size;
		}
		table[i].file_name = file_name;
		table[i].num_entries = get_file_size(file_name.c_str()) / size;
		
		file[i] = fopen(table[i].file_name.c_str(), "rb");
		
		std::cout << "Table " << (i + 1) << ": "
				<< table[i].num_entries << " entries" << std::endl;
	}
	
	for(int index = 0; index < 100; ++index)
	{
//		std::cout << std::endl;
		std::vector<uint32_t> proof;
		const auto y = gather_7(1000000000 + index, proof);
		
//		std::cout << y << " :";
//		for(auto x : proof) {
//			std::cout << " " << x;
//		}
//		std::cout << " (" << proof.size() << " x 32-bit)" << std::endl;
		
		uint8_t challenge[32] = {};
		chia::Bits(y, 32).ToBytes(challenge);
		
		uint8_t proof_bytes[256] = {};
		{
			size_t i = 0;
			for(auto x : proof) {
				chia::Bits(x, 32).ToBytes(proof_bytes + (4 * i++));
			}
		}
		{
			chia::LargeBits bits(proof_bytes, sizeof(proof_bytes), sizeof(proof_bytes) * 8);
//			std::cout << "proof = " << bits.ToString() << std::endl;
		}
		
		chia::Verifier verify;
		const auto qual = verify.ValidateProof(id, 32, challenge, proof_bytes, sizeof(proof_bytes));
		std::cout << "quality = " << qual.ToString() << std::endl;
	}
	
	return 0;
}

