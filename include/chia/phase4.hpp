/*
 * phase4.hpp
 *
 *  Created on: Jun 3, 2021
 *      Author: mad
 */

#include <chia/phase4.h>
#include <chia/DiskSort.hpp>

#include <chia/encoding.hpp>
#include <chia/util.hpp>


namespace phase4 {

// Calculates the size of one C3 park. This will store bits for each f7 between
// two C1 checkpoints, depending on how many times that f7 is present. For low
// values of k, we need extra space to account for the additional variability.
static uint32_t CalculateC3Size(uint8_t k)
{
	if (k < 20) {
		return Util::ByteAlign(8 * kCheckpoint1Interval) / 8;
	} else {
		return Util::ByteAlign(kC3BitsPerEntry * kCheckpoint1Interval) / 8;
	}
}

// Writes the checkpoint tables. The purpose of these tables, is to store a list of ~2^k values
// of size k (the proof of space outputs from table 7), in a way where they can be looked up for
// proofs, but also efficiently. To do this, we assume table 7 is sorted by f7, and we write the
// deltas between each f7 (which will be mostly 1s and 0s), with a variable encoding scheme
// (C3). Furthermore, we create C1 checkpoints along the way.  For example, every 10,000 f7
// entries, we can have a C1 checkpoint, and a C3 delta encoded entry with 10,000 deltas.

// Since we can't store all the checkpoints in
// memory for large plots, we create checkpoints for the checkpoints (C2), that are meant to be
// stored in memory during proving. For example, every 10,000 C1 entries, we can have a C2
// entry.

// The final table format for the checkpoints will be:
// C1 (checkpoint values)
// C2 (checkpoint values into)
// C3 (deltas of f7s between C1 checkpoints)
uint64_t compute(	FILE* plot_file,
					const uint8_t k, const int header_size,
					phase3::DiskSortNP* L_sort_7, int num_threads,
					const uint64_t final_pointer_7,
					const uint64_t final_entries_written)
{
	const uint32_t P7_park_size = Util::ByteAlign((k + 1) * kEntriesPerPark) / 8;
    const uint64_t number_of_p7_parks =
        ((final_entries_written == 0 ? 0 : final_entries_written - 1) / kEntriesPerPark) + 1;
    
    std::array<uint64_t, 12> final_table_begin_pointers = {};
    final_table_begin_pointers[7] = final_pointer_7;

    const uint64_t begin_byte_C1 = final_table_begin_pointers[7] + number_of_p7_parks * P7_park_size;

    const uint64_t total_C1_entries = cdiv(final_entries_written, kCheckpoint1Interval);
    const uint64_t begin_byte_C2 = begin_byte_C1 + (total_C1_entries + 1) * (Util::ByteAlign(k) / 8);
    const uint64_t total_C2_entries = cdiv(total_C1_entries, kCheckpoint2Interval);
    const uint64_t begin_byte_C3 = begin_byte_C2 + (total_C2_entries + 1) * (Util::ByteAlign(k) / 8);

    const uint32_t C3_size = CalculateC3Size(k);
    const uint64_t end_byte = begin_byte_C3 + total_C1_entries * C3_size;

    final_table_begin_pointers[8] = begin_byte_C1;
    final_table_begin_pointers[9] = begin_byte_C2;
    final_table_begin_pointers[10] = begin_byte_C3;
    final_table_begin_pointers[11] = end_byte;

    uint64_t final_file_writer_1 = begin_byte_C1;
    uint64_t final_file_writer_3 = final_table_begin_pointers[7];

    uint64_t prev_y = 0;
    uint64_t num_C1_entries = 0;
    
    std::vector<uintkx_t> C2;

    std::cout << "[P4] Starting to write C1 and C3 tables" << std::endl;
    
	struct park_deltas_t {
		uint64_t offset = 0;
		std::vector<uint8_t> deltas;
	} park_deltas;
	
	struct park_data_t {
		uint64_t offset = 0;
		std::vector<uintkx_t> array;	// new_pos
	} park_data;
	
    struct write_data_t {
		uint64_t offset = 0;
		std::vector<uint8_t> buffer;
	};
    
    Thread<std::vector<write_data_t>> plot_write(
		[plot_file](std::vector<write_data_t>& input) {
			for(const auto& write : input) {
				fwrite_at(plot_file, write.offset, write.buffer.data(), write.buffer.size());
			}
		}, "phase4/write");
    
    ThreadPool<std::vector<park_data_t>, std::vector<write_data_t>> p7_threads(
		[k, P7_park_size](std::vector<park_data_t>& input, std::vector<write_data_t>& out, size_t&) {
			for(const auto& park : input) {
				write_data_t tmp;
				tmp.offset = park.offset;
				tmp.buffer.resize(P7_park_size);
				ParkBits bits;
				for(uint64_t new_pos : park.array) {
					bits += ParkBits(new_pos, k + 1);
				}
				bits.ToBytes(tmp.buffer.data());
				out.emplace_back(std::move(tmp));
    		}
		}, &plot_write, std::max(num_threads / 2, 1), "phase4/P7");
    
	ThreadPool<park_deltas_t, std::vector<write_data_t>> park_threads(
		[C3_size](park_deltas_t& park, std::vector<write_data_t>& out, size_t&) {
			write_data_t tmp;
			tmp.offset = park.offset;
			tmp.buffer.resize(C3_size);
			const size_t num_bytes =
					Encoding::ANSEncodeDeltas(park.deltas, kC3R, tmp.buffer.data() + 2);
			
			if(num_bytes + 2 > C3_size) {
				throw std::logic_error("C3 overflow");
			}
			Util::IntToTwoBytes(tmp.buffer.data(), num_bytes);	// Write the size
			out.emplace_back(std::move(tmp));
		}, &plot_write, std::max(num_threads / 2, 1), "phase4/C3");

    // We read each table7 entry, which is sorted by f7, but we don't need f7 anymore. Instead,
	// we will just store pos6, and the deltas in table C3, and checkpoints in tables C1 and C2.
    Thread<std::pair<std::vector<phase3::entry_np>, size_t>> read_thread(
	[k, begin_byte_C3, C3_size, P7_park_size, &num_C1_entries, &prev_y, &C2,
	 &park_deltas, &park_data, &park_threads, &p7_threads, &plot_write,
	 &final_file_writer_1, &final_file_writer_3]
	 (std::pair<std::vector<phase3::entry_np>, size_t>& input) {
		std::vector<park_data_t> parks;
		parks.reserve(input.first.size() / kEntriesPerPark + 2);
		uint64_t index = input.second;
		for(const auto& entry : input.first) {
			const uint64_t entry_y = entry.key;
	
			if(index % kEntriesPerPark == 0 && index > 0)
			{
				park_data.offset = final_file_writer_3;
				final_file_writer_3 += P7_park_size;
				
				parks.emplace_back(std::move(park_data));
				
				park_data.array.clear();
				park_data.array.reserve(kEntriesPerPark);
			}
			park_data.array.push_back(entry.pos);
	
			if(index % kCheckpoint1Interval == 0)
			{
				write_data_t out;
				out.offset = final_file_writer_1;
				out.buffer.resize(Util::ByteAlign(k) / 8);
				Bits(entry_y, k).ToBytes(out.buffer.data());
				final_file_writer_1 += out.buffer.size();
				{
					std::vector<write_data_t> out_;
					out_.emplace_back(std::move(out));
					plot_write.take(out_);
				}
				if(num_C1_entries > 0) {
					park_deltas.offset = begin_byte_C3 + (num_C1_entries - 1) * C3_size;
					park_threads.take(park_deltas);
				}
				if(index % (kCheckpoint1Interval * kCheckpoint2Interval) == 0) {
					C2.push_back(entry_y);
				}
				park_deltas.deltas.clear();
				park_deltas.deltas.reserve(kCheckpoint1Interval);
				num_C1_entries++;
			}
			else {
				park_deltas.deltas.push_back(entry_y - prev_y);
			}
			prev_y = entry_y;
			index++;
		}
		p7_threads.take(parks);
	}, "phase4/read");
    
    L_sort_7->read(&read_thread, num_threads);
    read_thread.close();
    
    park_data.offset = final_file_writer_3;
    {
		std::vector<park_data_t> parks{park_data};
		p7_threads.take(parks);
    }
    final_file_writer_3 += P7_park_size;

    if(!park_deltas.deltas.empty()) {
    	park_deltas.offset = begin_byte_C3 + (num_C1_entries - 1) * C3_size;
		park_threads.take(park_deltas);
    }
    Encoding::ANSFree(kC3R);
    
    park_threads.close();
    p7_threads.close();
    plot_write.close();

    uint8_t C1_entry_buf[8] = {};
    Bits(0, Util::ByteAlign(k)).ToBytes(C1_entry_buf);
    final_file_writer_1 +=
    		fwrite_at(plot_file, final_file_writer_1, C1_entry_buf, Util::ByteAlign(k) / 8);
    
    std::cout << "[P4] Finished writing C1 and C3 tables" << std::endl;
    std::cout << "[P4] Writing C2 table" << std::endl;

    for(auto C2_entry : C2) {
        Bits(C2_entry, k).ToBytes(C1_entry_buf);
        final_file_writer_1 +=
        		fwrite_at(plot_file, final_file_writer_1, C1_entry_buf, Util::ByteAlign(k) / 8);
    }
    Bits(0, Util::ByteAlign(k)).ToBytes(C1_entry_buf);
    final_file_writer_1 +=
    		fwrite_at(plot_file, final_file_writer_1, C1_entry_buf, Util::ByteAlign(k) / 8);
    
    std::cout << "[P4] Finished writing C2 table" << std::endl;

    final_file_writer_1 = header_size - 8 * 3;
    uint8_t table_pointer_bytes[8] = {};

    // Writes the pointers to the start of the tables, for proving
    for (int i = 8; i <= 10; i++) {
        Util::IntToEightBytes(table_pointer_bytes, final_table_begin_pointers[i]);
        final_file_writer_1 +=
        		fwrite_at(plot_file, final_file_writer_1, table_pointer_bytes, 8);
    }
    return end_byte;
}

inline
void compute(	const phase3::output_t& input, output_t& out,
				const int num_threads, const int log_num_buckets,
				const std::string plot_name,
				const std::string tmp_dir,
				const std::string tmp_dir_2,
				const std::string plot_dir)
{
	const auto total_begin = get_wall_time_micros();
	
	FILE* plot_file = fopen(input.plot_file_name.c_str(), "rb+");
	if(!plot_file) {
		throw std::runtime_error("fopen() failed");
	}
	
	out.plot_size = compute(plot_file, input.params.k, input.header_size, input.sort_7.get(),
							num_threads, input.final_pointer_7, input.num_written_7);
	
	fclose(plot_file);
	
	out.params = input.params;
	out.plot_file_name = plot_dir + plot_name + ".plot";
	
	std::rename(input.plot_file_name.c_str(), out.plot_file_name.c_str());
	
	std::cout << "Phase 4 took " << (get_wall_time_micros() - total_begin) / 1e6 << " sec"
			", final plot size is " << out.plot_size << " bytes" << std::endl;
}


} // phase4
