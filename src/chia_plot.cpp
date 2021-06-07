/*
 * chia_plot.cpp
 *
 *  Created on: Jun 5, 2021
 *      Author: mad
 */

#include <chia/phase1.hpp>
#include <chia/phase2.hpp>
#include <chia/phase3.hpp>
#include <chia/phase4.hpp>
#include <chia/chia_filesystem.hpp>

#include <bls.hpp>
#include <sodium.h>

#include <chrono>
#include <iostream>


inline
std::vector<uint8_t> hex_to_bytes(const std::string& hex)
{
	std::vector<uint8_t> result;
	for(size_t i = 0; i < hex.length(); i += 2) {
		const std::string byteString = hex.substr(i, 2);
		result.push_back(::strtol(byteString.c_str(), NULL, 16));
	}
	return result;
}

inline
std::string get_date_string_ex(const char* format, bool UTC = false, int64_t time_secs = -1) {
	::time_t time_;
	if(time_secs < 0) {
		::time(&time_);
	} else {
		time_ = time_secs;
	}
	::tm* tmp;
	if(UTC) {
		tmp = ::gmtime(&time_);
	} else {
		tmp = ::localtime(&time_);
	}
	char buf[256];
	::strftime(buf, sizeof(buf), format, tmp);
	return std::string(buf);
}

inline
phase4::output_t create_plot(	const int num_threads,
								const int log_num_buckets,
								const vector<uint8_t>& pool_key_bytes,
								const vector<uint8_t>& farmer_key_bytes,
								const std::string& tmp_dir,
								const std::string& tmp_dir_2)
{
	const auto total_begin = get_wall_time_micros();
	
	std::cout << "Number of Threads: " << num_threads << std::endl;
	std::cout << "Number of Sort Buckets: 2^" << log_num_buckets
			<< " (" << (1 << log_num_buckets) << ")" << std::endl;
	
	const bls::G1Element pool_key = bls::G1Element::FromByteVector(pool_key_bytes);
	const bls::G1Element farmer_key = bls::G1Element::FromByteVector(farmer_key_bytes);
	
	std::cout << "Pool Public Key:   " << bls::Util::HexStr(pool_key.Serialize()) << std::endl;
	std::cout << "Farmer Public Key: " << bls::Util::HexStr(farmer_key.Serialize()) << std::endl;
	
	vector<uint8_t> seed(32);
	randombytes_buf(seed.data(), seed.size());
	
	bls::AugSchemeMPL MPL;
	const bls::PrivateKey master_sk = MPL.KeyGen(seed);
	
	bls::PrivateKey local_sk = master_sk;
	for(uint32_t i : {12381, 8444, 3, 0}) {
		local_sk = MPL.DeriveChildSk(local_sk, i);
	}
	const bls::G1Element local_key = local_sk.GetG1Element();
	const bls::G1Element plot_key = local_key + farmer_key;
	
	phase1::input_t params;
	{
		vector<uint8_t> bytes = pool_key.Serialize();
		{
			const auto plot_bytes = plot_key.Serialize();
			bytes.insert(bytes.end(), plot_bytes.begin(), plot_bytes.end());
		}
		bls::Util::Hash256(params.id.data(), bytes.data(), bytes.size());
	}
	const std::string plot_name = "plot-k32-" + get_date_string_ex("%Y-%m-%d-%H-%M")
			+ "-" + bls::Util::HexStr(params.id.data(), params.id.size());
	
	std::cout << "Working Directory:   " << (tmp_dir.empty() ? "$PWD" : tmp_dir) << std::endl;
	std::cout << "Working Directory 2: " << (tmp_dir_2.empty() ? "$PWD" : tmp_dir_2) << std::endl;
	std::cout << "Plot Name: " << plot_name << std::endl;
	
	// memo = bytes(pool_public_key) + bytes(farmer_public_key) + bytes(local_master_sk)
	params.memo.insert(params.memo.end(), pool_key_bytes.begin(), pool_key_bytes.end());
	params.memo.insert(params.memo.end(), farmer_key_bytes.begin(), farmer_key_bytes.end());
	{
		const auto bytes = master_sk.Serialize();
		params.memo.insert(params.memo.end(), bytes.begin(), bytes.end());
	}
	
	phase1::output_t out_1;
	phase1::compute(params, out_1, num_threads, log_num_buckets, plot_name, tmp_dir, tmp_dir_2);
	
	phase2::output_t out_2;
	phase2::compute(out_1, out_2, num_threads, log_num_buckets, plot_name, tmp_dir, tmp_dir_2);
	
	phase3::output_t out_3;
	phase3::compute(out_2, out_3, num_threads, log_num_buckets, plot_name, tmp_dir, tmp_dir_2);
	
	phase4::output_t out_4;
	phase4::compute(out_3, out_4, num_threads, log_num_buckets, plot_name, tmp_dir, tmp_dir_2);
	
	std::cout << "Total plot creation time was "
			<< (get_wall_time_micros() - total_begin) / 1e6 << " sec" << std::endl;
	return out_4;
}


int main(int argc, char** argv)
{
	if(argc < 3) {
		std::cout << "chia_plot <pool_key> <farmer_key> [tmp_dir] [tmp_dir2] [num_threads] [log_num_buckets]" << std::endl << std::endl;
		std::cout << "For <pool_key> and <farmer_key> see output of `chia keys show`." << std::endl;
		std::cout << "<tmp_dir> needs about 200G space, it will handle about 25% of all writes. (Examples: './', '/mnt/tmp/')" << std::endl;
		std::cout << "<tmp_dir2> needs about 110G space and ideally is a RAM drive, it will handle about 75% of all writes." << std::endl;
		std::cout << "If <tmp_dir> is not specified it defaults to current directory." << std::endl;
		std::cout << "If <tmp_dir2> is not specified it defaults to <tmp_dir>." << std::endl;
		return -1;
	}
	const auto pool_key = hex_to_bytes(argv[1]);
	const auto farmer_key = hex_to_bytes(argv[2]);
	const std::string tmp_dir = argc > 3 ? std::string(argv[3]) : std::string();
	const std::string tmp_dir2 = argc > 4 ? std::string(argv[4]) : tmp_dir;
	const int num_threads = argc > 5 ? atoi(argv[5]) : 4;
	const int log_num_buckets = argc > 6 ? atoi(argv[6]) : 7;
	
	if(pool_key.size() != bls::G1Element::SIZE) {
		std::cout << "Invalid <pool_key>: " << bls::Util::HexStr(pool_key) << std::endl;
		return -2;
	}
	if(farmer_key.size() != bls::G1Element::SIZE) {
		std::cout << "Invalid <farmer_key>: " << bls::Util::HexStr(farmer_key) << std::endl;
		return -2;
	}
	try {
		// Check if the paths exist
		if(!fs::exists(tmp_dir)) {
			throw std::runtime_error("<tmp_dir> directory '" + tmp_dir + "' does not exist");
		}
		if(!fs::exists(tmp_dir2)) {
			throw std::runtime_error("<tmp_dir2> directory '" + tmp_dir2 + "' does not exist");
		}
	}
	catch(const std::exception& ex) {
		std::cout << "Error: " << ex.what() << std::endl;
		return -2;
	}
	
	const auto out = create_plot(num_threads, log_num_buckets, pool_key, farmer_key, tmp_dir, tmp_dir2);
	
	// TODO: copy to destination
	
	return 0;
}


