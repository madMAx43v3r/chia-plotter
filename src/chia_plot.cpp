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

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif


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
								const std::string& tmp_dir_2,
								const std::string& final_dir)
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
	std::cout << "Final Directory: " << (final_dir.empty() ? "$PWD" : final_dir) << std::endl;
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
		std::cout << "If <final_dir> is not specified it defaults to <tmp_dir>" << std::endl;
		std::cout << "[num_threads] defaults to 4, it's recommended to use number of physical cores." << std::endl;
		std::cout << "[log_num_buckets] defaults to 7 (2^7 = 128)" << std::endl;
		return -1;
	}
	const auto pool_key = hex_to_bytes(argv[1]);
	const auto farmer_key = hex_to_bytes(argv[2]);
	const std::string tmp_dir = argc > 3 ? std::string(argv[3]) : std::string();
	const std::string tmp_dir2 = argc > 4 ? std::string(argv[4]) : tmp_dir;
	const std::string final_dir = argc > 5 ? std::string(argv[5]) : tmp_dir;
	const int num_threads = argc > 6 ? atoi(argv[6]) : 4;
	const int log_num_buckets = argc > 7 ? atoi(argv[7]) : 7;
	
	if(pool_key.size() != bls::G1Element::SIZE) {
		std::cout << "Invalid <pool_key>: " << bls::Util::HexStr(pool_key)
			<< " (needs to be " << bls::G1Element::SIZE << " bytes)" << std::endl;
		return -2;
	}
	if(farmer_key.size() != bls::G1Element::SIZE) {
		std::cout << "Invalid <farmer_key>: " << bls::Util::HexStr(farmer_key)
			<< " (needs to be " << bls::G1Element::SIZE << " bytes)" << std::endl;
		return -2;
	}
	if(!tmp_dir.empty() && tmp_dir.find_last_of("/\\") != tmp_dir.size() - 1) {
		std::cout << "Invalid <tmp_dir>: " << tmp_dir << " (needs trailing '/' or '\\')" << std::endl;
		return -2;
	}
	if(!tmp_dir2.empty() && tmp_dir2.find_last_of("/\\") != tmp_dir2.size() - 1) {
		std::cout << "Invalid <tmp_dir2>: " << tmp_dir2 << " (needs trailing '/' or '\\')" << std::endl;
		return -2;
	}
	if(!final_dir.empty() && final_dir.find_last_of("/\\") != final_dir.size() - 1) {
                std::cout << "Invalid <final_dir>: " << final_dir << " (needs trailing '/' or '\\')" << std::endl;
                return -2;
        }
	if(num_threads < 1 || num_threads > 1024) {
		std::cout << "Invalid num_threads: " << num_threads << " (supported: [1..1024])" << std::endl;
		return -2;
	}
	if(log_num_buckets < 4 || log_num_buckets > 16) {
		std::cout << "Invalid log_num_buckets: " << log_num_buckets << " (supported: 2^[4..16])" << std::endl;
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
		if (!fs::exists(final_dir)) {
                        throw InvalidValueException("<final_dir> directory " + final_dir + " does not exist");
                }

	}
	catch(const std::exception& ex) {
		std::cout << "Error: " << ex.what() << std::endl;
		return -2;
	}
	
	const auto out = create_plot(num_threads, log_num_buckets, pool_key, farmer_key, tmp_dir, tmp_dir2, final_dir);

	      //Copy to destination
        bool bCopied = false;
        bool bRenamed = false;
        std::string final_f = out.plot_file_name;
        final_f.erase(final_f.find_last_of("."), std::string::npos);
        fs::path final_tmp_filename = fs::path(final_dir) / fs::path(out.plot_file_name);
        fs::path final_filename = fs::path(final_dir) / fs::path(final_f);
        const auto copy_begin = get_wall_time_micros();
        do {
                std::error_code ec;
                if (tmp_dir == final_dir || tmp_dir2 == final_dir) {
                        fs::rename(out.plot_file_name,final_filename,ec);
                        if (ec.value() != 0) {
                                std::cout << "Could not rename " << out.plot_file_name << " to " << final_filename
                                         << ". Error " << ec.message() << ". Retrying in one minute." << std::endl;
                        } else {
                                bRenamed = true;
                                std::cout << "Renamed final file from " << out.plot_file_name << " to  " << final_filename << std::endl;
                        }
                } else {
                        if (!bCopied) {
                                  fs::copy(out.plot_file_name,final_tmp_filename,fs::copy_options::overwrite_existing, ec);
                                  if (ec.value() != 0) {
                                        std::cout << "Could not copy "  << out.plot_file_name << " to " << final_tmp_filename
                                                  << ". Error " << ec.message() << ". Retrying in one minute. " << std::endl;
                                  } else {
                                        std::cout << "Copied final file from "  << out.plot_file_name << " to " << final_tmp_filename << std::endl;
                                        std::cout << "Copy time =  " << (get_wall_time_micros() - copy_begin) / 1e6 << " sec" << std::endl;
                                        bCopied = true;
                                        bool removed = fs::remove(out.plot_file_name);
                                        std::cout << "Removed .tmp file " << out.plot_file_name << "? " << removed << std::endl;
                                  }
                        }

                }
                if (bCopied && (!bRenamed)) {
                        fs::rename(final_tmp_filename, final_filename, ec);
                        if (ec.value() !=0) {
                                std::cout << "Could not rename " << final_tmp_filename << " to " << final_filename
                                << ". Error " << ec.message() << ". Retrying in one minute." << std::endl;
                        } else {
                                std::cout << "Renamed final file from " << final_tmp_filename << " to " << final_filename << std::endl;
                                bRenamed = true;
                        }
                }
         if (!bRenamed) {
#ifdef _WIN32
                Sleep(1 * 60000);
#else
                sleep(1 * 60);
#endif
            }
        } while (!bRenamed);
	
	
	return 0;
}


