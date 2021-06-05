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
uint64_t compute(	FILE* plot_file, const int header_size,
					phase3::DiskSortNP* L_sort_7, int num_threads,
					const uint64_t final_pointer_7,
					const uint64_t final_entries_written)
{
    static constexpr uint8_t k = 32;
	
	const uint32_t P7_park_size = Util::ByteAlign((k + 1) * kEntriesPerPark) / 8;
    const uint64_t number_of_p7_parks =
        ((final_entries_written == 0 ? 0 : final_entries_written - 1) / kEntriesPerPark) +
        1;
    
    std::array<uint64_t, 12> final_table_begin_pointers = {};
    final_table_begin_pointers[7] = final_pointer_7;

    const uint64_t begin_byte_C1 = final_table_begin_pointers[7] + number_of_p7_parks * P7_park_size;

    const uint64_t total_C1_entries = cdiv(final_entries_written, kCheckpoint1Interval);
    const uint64_t begin_byte_C2 = begin_byte_C1 + (total_C1_entries + 1) * (Util::ByteAlign(k) / 8);
    const uint64_t total_C2_entries = cdiv(total_C1_entries, kCheckpoint2Interval);
    const uint64_t begin_byte_C3 = begin_byte_C2 + (total_C2_entries + 1) * (Util::ByteAlign(k) / 8);

    const uint32_t size_C3 = CalculateC3Size(k);
    const uint64_t end_byte = begin_byte_C3 + (total_C1_entries)*size_C3;

    final_table_begin_pointers[8] = begin_byte_C1;
    final_table_begin_pointers[9] = begin_byte_C2;
    final_table_begin_pointers[10] = begin_byte_C3;
    final_table_begin_pointers[11] = end_byte;

    uint64_t final_file_writer_1 = begin_byte_C1;
    uint64_t final_file_writer_2 = begin_byte_C3;
    uint64_t final_file_writer_3 = final_table_begin_pointers[7];

    uint64_t prev_y = 0;
    std::vector<Bits> C2;
    uint64_t num_C1_entries = 0;
    std::vector<uint8_t> deltas_to_write;

    auto C1_entry_buf = new uint8_t[Util::ByteAlign(k) / 8];
    auto C3_entry_buf = new uint8_t[size_C3];
    auto P7_entry_buf = new uint8_t[P7_park_size];

    std::cout << "[P4] Starting to write C1 and C3 tables" << std::endl;
    
    uint64_t f7_position = 0;
    ParkBits to_write_p7;

    // We read each table7 entry, which is sorted by f7, but we don't need f7 anymore. Instead,
	// we will just store pos6, and the deltas in table C3, and checkpoints in tables C1 and C2.
    Thread<std::vector<phase3::entry_np>> thread(
	[plot_file, P7_park_size, P7_entry_buf, C1_entry_buf, C3_entry_buf, &f7_position, &to_write_p7,
	 begin_byte_C3, size_C3, &deltas_to_write, &prev_y, &C2,
	 &num_C1_entries, &final_file_writer_1, &final_file_writer_2, &final_file_writer_3]
	 (std::vector<phase3::entry_np>& input) {
		for(const auto& entry : input) {
			const uint64_t entry_y = entry.key;
			const uint64_t entry_new_pos = entry.pos;
	
			Bits entry_y_bits = Bits(entry_y, k);
	
			if (f7_position % kEntriesPerPark == 0 && f7_position > 0) {
				memset(P7_entry_buf, 0, P7_park_size);
				to_write_p7.ToBytes(P7_entry_buf);
				final_file_writer_3 +=
						fwrite_at(plot_file, final_file_writer_3, (P7_entry_buf), P7_park_size);
				to_write_p7 = ParkBits();
			}
	
			to_write_p7 += ParkBits(entry_new_pos, k + 1);
	
			if (f7_position % kCheckpoint1Interval == 0) {
				entry_y_bits.ToBytes(C1_entry_buf);
				final_file_writer_1 +=
						fwrite_at(plot_file, final_file_writer_1, (C1_entry_buf), Util::ByteAlign(k) / 8);
				
				if (num_C1_entries > 0) {
					final_file_writer_2 = begin_byte_C3 + (num_C1_entries - 1) * size_C3;
					size_t num_bytes =
						Encoding::ANSEncodeDeltas(deltas_to_write, kC3R, C3_entry_buf + 2) + 2;
	
					// We need to be careful because deltas are variable sized, and they need to fit
					assert(size_C3 * 8 > num_bytes);
	
					// Write the size
					Util::IntToTwoBytes(C3_entry_buf, num_bytes - 2);
	
					final_file_writer_2 +=
							fwrite_at(plot_file, final_file_writer_2, (C3_entry_buf), num_bytes);
				}
				prev_y = entry_y;
				if (f7_position % (kCheckpoint1Interval * kCheckpoint2Interval) == 0) {
					C2.emplace_back(std::move(entry_y_bits));
				}
				deltas_to_write.clear();
				++num_C1_entries;
			} else {
				deltas_to_write.push_back(entry_y - prev_y);
				prev_y = entry_y;
			}
			f7_position++;
		}
	}, "phase4/final");
    
    L_sort_7->read(&thread, num_threads);
    thread.close();
    
    Encoding::ANSFree(kC3R);

    // Writes the final park to disk
    memset(P7_entry_buf, 0, P7_park_size);
    to_write_p7.ToBytes(P7_entry_buf);

    final_file_writer_3 += fwrite_at(plot_file, final_file_writer_3, (P7_entry_buf), P7_park_size);

    if (!deltas_to_write.empty()) {
        size_t num_bytes = Encoding::ANSEncodeDeltas(deltas_to_write, kC3R, C3_entry_buf + 2);
        memset(C3_entry_buf + num_bytes + 2, 0, size_C3 - (num_bytes + 2));
        final_file_writer_2 = begin_byte_C3 + (num_C1_entries - 1) * size_C3;

        // Write the size
        Util::IntToTwoBytes(C3_entry_buf, num_bytes);

        final_file_writer_2 +=
        		fwrite_at(plot_file, final_file_writer_2, (C3_entry_buf), size_C3);
        Encoding::ANSFree(kC3R);
    }

    Bits(0, Util::ByteAlign(k)).ToBytes(C1_entry_buf);
    final_file_writer_1 +=
    		fwrite_at(plot_file, final_file_writer_1, (C1_entry_buf), Util::ByteAlign(k) / 8);
    
    std::cout << "[P4] Finished writing C1 and C3 tables" << std::endl;
    std::cout << "[P4] Writing C2 table" << std::endl;

    for (Bits &C2_entry : C2) {
        C2_entry.ToBytes(C1_entry_buf);
        final_file_writer_1 +=
        		fwrite_at(plot_file, final_file_writer_1, (C1_entry_buf), Util::ByteAlign(k) / 8);
    }
    Bits(0, Util::ByteAlign(k)).ToBytes(C1_entry_buf);
    final_file_writer_1 +=
    		fwrite_at(plot_file, final_file_writer_1, (C1_entry_buf), Util::ByteAlign(k) / 8);
    
    std::cout << "[P4] Finished writing C2 table" << std::endl;

    delete[] C3_entry_buf;
    delete[] C1_entry_buf;
    delete[] P7_entry_buf;

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
				const std::string tmp_dir_2)
{
	const auto total_begin = get_wall_time_micros();
	
	out.params = input.params;
	out.plot_file_name = input.plot_file_name;
	
	FILE* plot_file = fopen(input.plot_file_name.c_str(), "r+");
	if(!plot_file) {
		throw std::runtime_error("fopen() failed");
	}
	
	out.plot_size = compute(plot_file, input.header_size, input.sort_7.get(),
							num_threads, input.final_pointer_7, input.num_written_7);
	
	fclose(plot_file);
	
	std::cout << "Phase 4 took " << (get_wall_time_micros() - total_begin) / 1e6 << " sec"
			", final plot size is " << out.plot_size << " bytes" << std::endl;
}


} // phase4
