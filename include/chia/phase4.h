/*
 * phase4.h
 *
 *  Created on: Jun 2, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_PHASE4_H_
#define INCLUDE_CHIA_PHASE4_H_

#include <chia/phase3.h>


namespace phase4 {

uint64_t compute(	FILE* plot_file, const int header_size,
					phase3::DiskSortNP* L_sort_7,
					const uint64_t final_pointer_7,
					const uint64_t final_entries_written);


} // phase4

#endif /* INCLUDE_CHIA_PHASE4_H_ */
