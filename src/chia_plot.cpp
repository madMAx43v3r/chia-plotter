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
phase4::output_t create_plot(	const int num_threads, const int log_num_buckets,
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
	bls::PrivateKey sk = MPL.KeyGen(seed);
	for(uint32_t i : {12381, 8444, 3, 0}) {
		sk = MPL.DeriveChildSk(sk, i);
	}
	const bls::G1Element local_key = sk.GetG1Element();
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
	const std::string plot_name = "plot-k32-" + get_date_string_ex("%Y-%m-%d-%H-%M-%S")
			+ "-" + bls::Util::HexStr(params.id.data(), params.id.size());
	
	std::cout << "Plot Name: " << plot_name << std::endl;
	
	// TODO: memo
	
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
		std::cout << "Usage: chia_plot <pool_key> <farmer_key> [num_threads] [log_num_buckets]" << std::endl;
		return -1;
	}
	const auto pool_key = hex_to_bytes(argv[1]);
	const auto farmer_key = hex_to_bytes(argv[2]);
	const int num_threads = argc > 3 ? atoi(argv[3]) : 4;
	const int log_num_buckets = argc > 4 ? atoi(argv[4]) : 7;
	
	const auto out = create_plot(num_threads, log_num_buckets, pool_key, farmer_key, "", "");
	
	// TODO: copy to destination
	
	return 0;
}


