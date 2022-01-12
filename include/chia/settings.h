/*
 * settings.h
 *
 *  Created on: Jun 7, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_SETTINGS_H_
#define INCLUDE_CHIA_SETTINGS_H_

#include <cstdint>
#include <cstddef>


/*
 * Number of table entries to read at once.
 * default = 65536
 */
extern size_t g_read_chunk_size;

/*
 * Number of table entries to buffer before writing to disk.
 * default = 4096
 */
extern size_t g_write_chunk_size;

namespace phase2 {
  extern int g_thread_multi;
}


#endif /* INCLUDE_CHIA_SETTINGS_H_ */
