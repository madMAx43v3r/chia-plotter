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
#include <chia/util.hpp>
#include <chia/copy.h>

#include <bls.hpp>
#include <sodium.h>
#include <cxxopts.hpp>
#include <libbech32.h>

#include <string>
#include <csignal>

#ifndef _WIN32
#include <sys/resource.h>
#endif

#ifdef __linux__ 
	#include <unistd.h>
	#define GETPID getpid
#elif _WIN32
	#include <processthreadsapi.h>
	#define GETPID GetCurrentProcessId
#else
	#define GETPID() int(-1)
#endif

bool gracefully_exit = false;
int64_t interrupt_timestamp = 0;

static void interrupt_handler(int sig)
{	
	if ( ( (get_wall_time_micros() - interrupt_timestamp) / 1e6) <= 1 ) {
		std::cout << std::endl << "Double Ctrl-C pressed, exiting now!" << std::endl;
		exit(-4);
	} else {
		interrupt_timestamp = get_wall_time_micros();
	}
    if (!gracefully_exit) {
    	std::cout << std::endl;
    	std::cout << "****************************************************************************************" << std::endl;
    	std::cout << "**  The crafting of plots will stop after the creation and copy of the current plot.  **" << std::endl;
    	std::cout << "**         !! If you want to force quit now, press Ctrl-C twice in series !!          **" << std::endl;
    	std::cout << "****************************************************************************************" << std::endl;
    	gracefully_exit = true;
    }
}

std::vector<uint8_t> bech32_address_decode(const std::string& addr)
{
	const auto res = bech32::decode(addr);
	if(res.encoding != bech32::Bech32m) {
		throw std::logic_error("invalid contract address (!Bech32m): " + addr);
	}
	if(res.dp.size() != 52) {
		throw std::logic_error("invalid contract address (size != 52): " + addr);
	}
	Bits bits;
	for(int i = 0; i < 51; ++i) {
		bits.AppendValue(res.dp[i], 5);
	}
	bits.AppendValue(res.dp[51] >> 4, 1);
	if(bits.GetSize() != 32 * 8) {
		throw std::logic_error("invalid contract address (bits != 256): " + addr);
	}
	std::vector<uint8_t> hash(32);
	bits.ToBytes(hash.data());
	return hash;
}

inline
phase4::output_t create_plot(	const int num_threads,
								const int log_num_buckets,
								const int log_num_buckets_3,
								const vector<uint8_t>& pool_key_bytes,
								const vector<uint8_t>& puzzle_hash_bytes,
								const vector<uint8_t>& farmer_key_bytes,
								const std::string& tmp_dir,
								const std::string& tmp_dir_2)
{
	const auto total_begin = get_wall_time_micros();
	const bool have_puzzle = !puzzle_hash_bytes.empty();
	
	std::cout << "Process ID: " << GETPID() << std::endl;
	std::cout << "Number of Threads: " << num_threads << std::endl;
	std::cout << "Number of Buckets P1:    2^" << log_num_buckets
			<< " (" << (1 << log_num_buckets) << ")" << std::endl;
	std::cout << "Number of Buckets P3+P4: 2^" << log_num_buckets_3
			<< " (" << (1 << log_num_buckets_3) << ")" << std::endl;
	
	bls::G1Element pool_key;
	bls::G1Element farmer_key;
	if(!have_puzzle) {
		try {
			pool_key = bls::G1Element::FromByteVector(pool_key_bytes);
		} catch(std::exception& ex) {
			std::cout << "Invalid poolkey: " << bls::Util::HexStr(pool_key_bytes) << std::endl;
			throw;
		}
	}
	try {
		farmer_key = bls::G1Element::FromByteVector(farmer_key_bytes);
	} catch(std::exception& ex) {
		std::cout << "Invalid farmerkey: " << bls::Util::HexStr(farmer_key_bytes) << std::endl;
		throw;
	}
	if(have_puzzle) {
		std::cout << "Pool Puzzle Hash:  " << bls::Util::HexStr(puzzle_hash_bytes) << std::endl;
	} else {
		std::cout << "Pool Public Key:   " << bls::Util::HexStr(pool_key.Serialize()) << std::endl;
	}
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
	
	bls::G1Element plot_key;
	if(have_puzzle) {
		vector<uint8_t> bytes = (local_key + farmer_key).Serialize();
		{
			const auto more_bytes = local_key.Serialize();
			bytes.insert(bytes.end(), more_bytes.begin(), more_bytes.end());
		}
		{
			const auto more_bytes = farmer_key.Serialize();
			bytes.insert(bytes.end(), more_bytes.begin(), more_bytes.end());
		}
		std::vector<uint8_t> hash(32);
		bls::Util::Hash256(hash.data(), bytes.data(), bytes.size());
		
		const auto taproot_sk = MPL.KeyGen(hash);
		plot_key = local_key + farmer_key + taproot_sk.GetG1Element();
	}
	else {
		plot_key = local_key + farmer_key;
	}
	
	phase1::input_t params;
	{
		vector<uint8_t> bytes = have_puzzle ? puzzle_hash_bytes : pool_key.Serialize();
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
	
	if(have_puzzle) {
		params.memo.insert(params.memo.end(), puzzle_hash_bytes.begin(), puzzle_hash_bytes.end());
	} else {
		params.memo.insert(params.memo.end(), pool_key_bytes.begin(), pool_key_bytes.end());
	}
	params.memo.insert(params.memo.end(), farmer_key_bytes.begin(), farmer_key_bytes.end());
	{
		const auto bytes = master_sk.Serialize();
		params.memo.insert(params.memo.end(), bytes.begin(), bytes.end());
	}
	params.plot_name = plot_name;
	
	phase1::output_t out_1;
	phase1::compute(params, out_1, num_threads, log_num_buckets, plot_name, tmp_dir, tmp_dir_2);
	
	phase2::output_t out_2;
	phase2::compute(out_1, out_2, num_threads, log_num_buckets_3, plot_name, tmp_dir, tmp_dir_2);
	
	phase3::output_t out_3;
	phase3::compute(out_2, out_3, num_threads, log_num_buckets_3, plot_name, tmp_dir, tmp_dir_2);
	
	phase4::output_t out_4;
	phase4::compute(out_3, out_4, num_threads, log_num_buckets_3, plot_name, tmp_dir, tmp_dir_2);
	
	const auto time_secs = (get_wall_time_micros() - total_begin) / 1e6;
	std::cout << "Total plot creation time was "
			<< time_secs << " sec (" << time_secs / 60. << " min)" << std::endl;
	return out_4;
}


int main(int argc, char** argv)
{

	cxxopts::Options options("chia_plot",
		"Multi-threaded pipelined Chia k32 plotter"
#ifdef GIT_COMMIT_HASH
		" - " GIT_COMMIT_HASH
#endif
		"\n\n"
		"For <poolkey> and <farmerkey> see output of `chia keys show`.\n"
		"To plot for pools, specify <contract> address via -c instead of <poolkey>, see `chia plotnft show`.\n"
		"<tmpdir> needs about 220 GiB space, it will handle about 25% of all writes. (Examples: './', '/mnt/tmp/')\n"
		"<tmpdir2> needs about 110 GiB space and ideally is a RAM drive, it will handle about 75% of all writes.\n"
		"Combined (tmpdir + tmpdir2) peak disk usage is less than 256 GiB.\n"
		"In case of <count> != 1, you may press Ctrl-C for graceful termination after current plot is finished,\n"
		"or double press Ctrl-C to terminate immediately.\n\n"
		"(Sponsored by Flexpool.io - Check them out if you're looking for a secure and scalable Chia pool)\n"
	);
	
	std::string pool_key_str;
	std::string contract_addr_str;
	std::string farmer_key_str;
	std::string tmp_dir;
	std::string tmp_dir2;
	std::string final_dir;
	int num_plots = 1;
	int num_threads = 4;
	int num_buckets = 256;
	int num_buckets_3 = 0;
	bool waitforcopy = false;
	bool tmptoggle = false;
	
	options.allow_unrecognised_options().add_options()(
		"n, count", "Number of plots to create (default = 1, -1 = infinite)", cxxopts::value<int>(num_plots))(
		"r, threads", "Number of threads (default = 4)", cxxopts::value<int>(num_threads))(
		"u, buckets", "Number of buckets (default = 256)", cxxopts::value<int>(num_buckets))(
		"v, buckets3", "Number of buckets for phase 3+4 (default = buckets)", cxxopts::value<int>(num_buckets_3))(
		"t, tmpdir", "Temporary directory, needs ~220 GiB (default = $PWD)", cxxopts::value<std::string>(tmp_dir))(
		"2, tmpdir2", "Temporary directory 2, needs ~110 GiB [RAM] (default = <tmpdir>)", cxxopts::value<std::string>(tmp_dir2))(
		"d, finaldir", "Final directory (default = <tmpdir>)", cxxopts::value<std::string>(final_dir))(
		"w, waitforcopy", "Wait for copy to start next plot", cxxopts::value<bool>(waitforcopy))(
		"p, poolkey", "Pool Public Key (48 bytes)", cxxopts::value<std::string>(pool_key_str))(
		"c, contract", "Pool Contract Address (62 chars)", cxxopts::value<std::string>(contract_addr_str))(
		"f, farmerkey", "Farmer Public Key (48 bytes)", cxxopts::value<std::string>(farmer_key_str))(
		"G, tmptoggle", "Alternate tmpdir/tmpdir2", cxxopts::value<bool>(tmptoggle))(
		"K, rmulti2", "Thread multiplier for P2 (default = 1)", cxxopts::value<int>(phase2::g_thread_multi))(
		"help", "Print help");
	
	if(argc <= 1) {
		std::cout << options.help({""}) << std::endl;
		return 0;
	}
	const auto args = options.parse(argc, argv);
	
	if(args.count("help")) {
		std::cout << options.help({""}) << std::endl;
		return 0;
	}
	if(contract_addr_str.empty() && pool_key_str.empty()) {
		std::cout << "Pool Public Key (for solo farming) or Pool Contract Address (for pool farming) needs to be specified via -p or -c, see `chia_plot --help`." << std::endl;
		return -2;
	}
	if(!contract_addr_str.empty() && !pool_key_str.empty()) {
		std::cout << "Choose either Pool Public Key (for solo farming) or Pool Contract Address (for pool farming), see `chia_plot --help`." << std::endl;
		return -2;
	}
	if(farmer_key_str.empty()) {
		std::cout << "Farmer Public Key (48 bytes) needs to be specified via -f, see `chia keys show`." << std::endl;
		return -2;
	}
	if(tmp_dir.empty()) {
		std::cout << "tmpdir needs to be specified via -t path/" << std::endl;
		return -2;
	}
	if(tmp_dir2.empty()) {
		tmp_dir2 = tmp_dir;
	}
	if(final_dir.empty()) {
		final_dir = tmp_dir;
	}
	if(num_buckets_3 <= 0) {
		num_buckets_3 = num_buckets;
	}
	std::vector<uint8_t> pool_key;
	std::vector<uint8_t> puzzle_hash;
	const auto farmer_key = hex_to_bytes(farmer_key_str);
	const int log_num_buckets = num_buckets >= 16 ? int(log2(num_buckets)) : num_buckets;
	const int log_num_buckets_3 = num_buckets_3 >= 16 ? int(log2(num_buckets_3)) : num_buckets_3;

	if(contract_addr_str.empty()) {
		pool_key = hex_to_bytes(pool_key_str);
		if(pool_key.size() != bls::G1Element::SIZE) {
			std::cout << "Invalid poolkey: " << bls::Util::HexStr(pool_key) << ", '" << pool_key_str
				<< "' (needs to be " << bls::G1Element::SIZE << " bytes, see `chia keys show`)" << std::endl;
			return -2;
		}
	}
	else {
		try {
			puzzle_hash = bech32_address_decode(contract_addr_str);
			if(puzzle_hash.size() != 32) {
				throw std::logic_error("pool puzzle hash needs to be 32 bytes");
			}
		}
		catch(std::exception& ex) {
			std::cout << "Invalid contract (address): 0x"
					<< bls::Util::HexStr(puzzle_hash) << ", '" << contract_addr_str
					<< "' (" << ex.what() << ", see `chia plotnft show`)" << std::endl;
			return -2;
		}
	}
	if(farmer_key.size() != bls::G1Element::SIZE) {
		std::cout << "Invalid farmerkey: " << bls::Util::HexStr(farmer_key) << ", '" << farmer_key_str
			<< "' (needs to be " << bls::G1Element::SIZE << " bytes, see `chia keys show`)" << std::endl;
		return -2;
	}
	if(!tmp_dir.empty() && tmp_dir.find_last_of("/\\") != tmp_dir.size() - 1) {
		std::cout << "Invalid tmpdir: " << tmp_dir << " (needs trailing '/' or '\\')" << std::endl;
		return -2;
	}
	if(!tmp_dir2.empty() && tmp_dir2.find_last_of("/\\") != tmp_dir2.size() - 1) {
		std::cout << "Invalid tmpdir2: " << tmp_dir2 << " (needs trailing '/' or '\\')" << std::endl;
		return -2;
	}
	if(!final_dir.empty() && final_dir.find_last_of("/\\") != final_dir.size() - 1) {
		std::cout << "Invalid finaldir: " << final_dir << " (needs trailing '/' or '\\')" << std::endl;
		return -2;
	}
	if(num_threads < 1 || num_threads > 1024) {
		std::cout << "Invalid threads parameter: " << num_threads << " (supported: [1..1024])" << std::endl;
		return -2;
	}
	if(log_num_buckets < 4 || log_num_buckets > 16) {
		std::cout << "Invalid buckets parameter -u: 2^" << log_num_buckets << " (supported: 2^[4..16])" << std::endl;
		return -2;
	}
	if (log_num_buckets_3 < 4 || log_num_buckets_3 > 16) {
		std::cout << "Invalid buckets parameter -v: 2^" << log_num_buckets_3 << " (supported: 2^[4..16])" << std::endl;
		return -2;
	}
	{
		const std::string path = tmp_dir + ".chia_plot_tmp";
		if(auto file = fopen(path.c_str(), "wb")) {
			fclose(file);
			remove(path.c_str());
		} else {
			std::cout << "Failed to write to tmpdir directory: '" << tmp_dir << "'" << std::endl;
			return -2;
		}
	}
	{
		const std::string path = tmp_dir2 + ".chia_plot_tmp2";
		if(auto file = fopen(path.c_str(), "wb")) {
			fclose(file);
			remove(path.c_str());
		} else {
			std::cout << "Failed to write to tmpdir2 directory: '" << tmp_dir2 << "'" << std::endl;
			return -2;
		}
	}
	{
		const std::string path = final_dir + ".chia_plot_final";
		if(auto file = fopen(path.c_str(), "wb")) {
			fclose(file);
			remove(path.c_str());
		} else {
			std::cout << "Failed to write to finaldir directory: '" << final_dir << "'" << std::endl;
			return -2;
		}
	}
	const int num_files_max = (1 << std::max(log_num_buckets, log_num_buckets_3)) + 2 * num_threads + 32;
	
#ifndef _WIN32
	if(false) {
		// try to increase the open file limit
		::rlimit the_limit;
		the_limit.rlim_cur = num_files_max + 10;
		the_limit.rlim_max = num_files_max + 10;
		if(setrlimit(RLIMIT_NOFILE, &the_limit)) {
			std::cout << "Warning: setrlimit() failed!" << std::endl;
		}
	}
#endif
	
	{
		// check that we can open required amount of files
		std::vector<std::pair<FILE*, std::string>> files;
		for(int i = 0; i < num_files_max; ++i) {
			const std::string path = tmp_dir + ".chia_plot_tmp." + std::to_string(i);
			if(auto file = fopen(path.c_str(), "wb")) {
				files.emplace_back(file, path);
			} else {
				std::cout << "Cannot open at least " << num_files_max
						<< " files, please raise maximum open file limit in OS." << std::endl;
				return -2;
			}
		}
		for(const auto& entry : files) {
			fclose(entry.first);
			remove(entry.second.c_str());
		}
	}

	if(num_plots > 1 || num_plots < 0) {
		std::signal(SIGINT, interrupt_handler);
		std::signal(SIGTERM, interrupt_handler);
	}
	
	std::cout << "Multi-threaded pipelined Chia k32 plotter"; 
	#ifdef GIT_COMMIT_HASH
		std::cout << " - " << GIT_COMMIT_HASH;
	#endif	
	std::cout << std::endl;
	std::cout << "(Sponsored by Flexpool.io - Check them out if you're looking for a secure and scalable Chia pool)" << std::endl << std::endl;
	std::cout << "Final Directory: " << final_dir << std::endl;
	if(num_plots >= 0) {
		std::cout << "Number of Plots: " << num_plots << std::endl;
	} else {
		std::cout << "Number of Plots: infinite" << std::endl;
	}
	
	Thread<std::pair<std::string, std::string>> copy_thread(
		[](std::pair<std::string, std::string>& from_to) {
			const auto total_begin = get_wall_time_micros();
			while(true) {
				try {
					const auto bytes = final_copy(from_to.first, from_to.second);
					
					const auto time = (get_wall_time_micros() - total_begin) / 1e6;
					if(time > 1) {
						std::cout << "Copy to " << from_to.second << " finished, took " << time << " sec, "
							<< ((bytes / time) / 1024 / 1024) << " MB/s avg." << std::endl;
					} else {
						std::cout << "Renamed final plot to " << from_to.second << std::endl;
					}
					break;
				} catch(const std::exception& ex) {
					std::cout << "Copy to " << from_to.second << " failed with: " << ex.what() << std::endl;
					std::this_thread::sleep_for(std::chrono::minutes(5));
				}
			}
		}, "final/copy");
	
	for(int i = 0; i < num_plots || num_plots < 0; ++i)
	{
		if (gracefully_exit) {
			std::cout << std::endl << "Process has been interrupted, waiting for copy/rename operations to finish ..." << std::endl;
			break;
		}
		std::cout << "Crafting plot " << i+1 << " out of " << num_plots << std::endl;
		const auto out = create_plot(
				num_threads, log_num_buckets, log_num_buckets_3,
				pool_key, puzzle_hash, farmer_key, tmp_dir, tmp_dir2);
		
		if(final_dir != tmp_dir)
		{
			const auto dst_path = final_dir + out.params.plot_name + ".plot";
			std::cout << "Started copy to " << dst_path << std::endl;
			copy_thread.take_copy(std::make_pair(out.plot_file_name, dst_path));
			if(waitforcopy) {
				copy_thread.wait();
			}
		}
		else if(tmptoggle) {
			final_dir = tmp_dir2;
		}
		if (tmptoggle) {
			tmp_dir.swap(tmp_dir2);
		}
	}
	copy_thread.close();
	
	return 0;
}
