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

#include <iostream>


phase4::output_t create_plot(	phase1::input_t& params,
								const int num_threads, const int log_num_buckets,
								const std::string plot_name,
								const std::string tmp_dir,
								const std::string tmp_dir_2)
{
	phase1::output_t out_1;
	phase1::compute(params, out_1, num_threads, log_num_buckets, plot_name, tmp_dir, tmp_dir_2);
	
	phase2::output_t out_2;
	phase2::compute(out_1, out_2, num_threads, log_num_buckets, plot_name, tmp_dir, tmp_dir_2);
	
	phase3::output_t out_3;
	phase3::compute(out_2, out_3, num_threads, log_num_buckets, plot_name, tmp_dir, tmp_dir_2);
	
	phase4::output_t out_4;
	phase4::compute(out_3, out_4, num_threads, log_num_buckets, plot_name, tmp_dir, tmp_dir_2);
	return out_4;
}


int main(int argc, char** argv)
{
	const int num_threads = argc > 1 ? atoi(argv[1]) : 4;
	const int log_num_buckets = argc > 2 ? atoi(argv[2]) : 7;
	
	const auto total_begin = get_wall_time_micros();
	
	std::cout << "Number of threads: " << num_threads << std::endl;
	std::cout << "Number of sort buckets: 2^" << log_num_buckets
			<< " (" << (1 << log_num_buckets) << ")" << std::endl;
	
	phase1::input_t params;
	
	for(size_t i = 0; i < params.id.size(); ++i) {
		params.id[i] = i + 1;
	}
	
	const auto out = create_plot(params, num_threads, log_num_buckets, "test", "", "");
	
	std::cout << "Total plot creation time was "
			<< (get_wall_time_micros() - total_begin) / 1e6 << " sec";
	return 0;
}


