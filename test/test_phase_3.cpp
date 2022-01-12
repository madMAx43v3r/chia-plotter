/*
 * test_phase_3.cpp
 *
 *  Created on: May 30, 2021
 *      Author: mad
 */

#include <chia/phase3.hpp>
#include <chia/DiskSort.hpp>
#include <chia/DiskTable.h>

#include <iostream>

using namespace phase3;


int main(int argc, char** argv)
{
	const int num_threads = argc > 1 ? atoi(argv[1]) : 4;
	const int log_num_buckets = argc > 2 ? atoi(argv[2]) : 7;
	
	const int k = 32;
	const auto total_begin = get_wall_time_micros();
	
	uint8_t id[32] = {};
	for(size_t i = 0; i < sizeof(id); ++i) {
		id[i] = i + 1;
	}
	
	table_t table_1;
	table_1.file_name = "test.p1.table1.tmp";
	table_1.num_entries = get_file_size(table_1.file_name.c_str()) / phase2::entry_1::disk_size;
	
	table_t table_7;
	table_7.file_name = "test.p2.table7.tmp";
	table_7.num_entries = get_file_size(table_7.file_name.c_str()) / phase2::entry_7::disk_size;
	
	bitfield bitfield_1(table_1.num_entries);
	{
		FILE* file = fopen("test.p2.bitfield1.tmp", "rb");
		if(!file) {
			throw std::runtime_error("bitfield1 missing");
		}
		bitfield_1.read(file);
		fclose(file);
	}
	
	FILE* plot_file = fopen("test.plot.tmp", "wb");
	if(!plot_file) {
		throw std::runtime_error("fopen() failed");
	}
	const uint32_t header_size = WriteHeader(plot_file, k, id, nullptr, 0);
	
	std::vector<uint64_t> final_pointers(8, 0);
	final_pointers[1] = header_size;
	
	uint64_t num_written_final = 0;
	
	DiskTable<phase2::entry_1> L_table_1(table_1.file_name, table_1.num_entries);
	
	auto R_sort_in = std::make_shared<phase2::DiskSortT>(
			k, log_num_buckets, "test.p2.t2", true);
	auto R_sort_lp = std::make_shared<DiskSortLP>(
			2 * k - 1, log_num_buckets, "test.p3s1.t2");
	
	compute_stage1<phase2::entry_1, phase2::entry_x, DiskSortNP, phase2::DiskSortT>(
			1, num_threads, nullptr, R_sort_in.get(), R_sort_lp.get(), &L_table_1, &bitfield_1);
	
	auto L_sort_np = std::make_shared<DiskSortNP>(
			k, log_num_buckets, "test.p3s2.t2", false);
	
	num_written_final += compute_stage2(
			1, k, num_threads, R_sort_lp.get(), L_sort_np.get(),
			plot_file, final_pointers[1], &final_pointers[2]);
	
	for(int L_index = 2; L_index < 6; ++L_index)
	{
		const std::string R_t = "t" + std::to_string(L_index + 1);
		
		R_sort_in = std::make_shared<phase2::DiskSortT>(
				k, log_num_buckets, "test.p2." + R_t, true);
		R_sort_lp = std::make_shared<DiskSortLP>(
				2 * k - 1, log_num_buckets, "test.p3s1." + R_t);
		
		compute_stage1<entry_np, phase2::entry_x, DiskSortNP, phase2::DiskSortT>(
				L_index, num_threads, L_sort_np.get(), R_sort_in.get(), R_sort_lp.get());
		
		L_sort_np = std::make_shared<DiskSortNP>(
				k, log_num_buckets, "test.p3s2." + R_t, false);
		
		num_written_final += compute_stage2(
				L_index, k, num_threads, R_sort_lp.get(), L_sort_np.get(),
				plot_file, final_pointers[L_index], &final_pointers[L_index + 1]);
	}
	
	DiskTable<phase2::entry_7> R_table_7(table_7.file_name, table_7.num_entries);
	
	R_sort_in = nullptr;
	R_sort_lp = std::make_shared<DiskSortLP>(2 * k - 1, log_num_buckets, "test.p3s1.t7");
	
	compute_stage1<entry_np, phase2::entry_7, DiskSortNP, phase2::DiskSort7>(
			6, num_threads, L_sort_np.get(), nullptr, R_sort_lp.get(), nullptr, nullptr, &R_table_7);
	
	L_sort_np = std::make_shared<DiskSortNP>(k, log_num_buckets, "test.p3s2.t7");
	L_sort_np->set_keep_files(true);
	
	const auto num_written_final_7 = compute_stage2(
			6, k, num_threads, R_sort_lp.get(), L_sort_np.get(),
			plot_file, final_pointers[6], &final_pointers[7]);
	num_written_final += num_written_final_7;
	
	fseek_set(plot_file, header_size - 10 * 8);
	for(size_t i = 1; i < final_pointers.size(); ++i) {
		uint8_t tmp[8] = {};
		Util::IntToEightBytes(tmp, final_pointers[i]);
		fwrite_ex(plot_file, tmp, sizeof(tmp));
	}
	fclose(plot_file);
	{
		std::ofstream out("test.p3.header_size");
		out << header_size << std::endl;
	}
	{
		std::ofstream out("test.p3.final_pointer_7");
		out << final_pointers[7] << std::endl;
	}
	{
		std::ofstream out("test.p3.num_written_final_7");
		out << num_written_final_7 << std::endl;
	}
	
	std::cout << "Phase 3 took " << (get_wall_time_micros() - total_begin) / 1e6 << " sec"
			", wrote " << num_written_final << " entries to final plot" << std::endl;
	return 0;
}



