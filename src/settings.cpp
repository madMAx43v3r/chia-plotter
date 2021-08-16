/*
 * settings.cpp
 *
 *  Created on: Jun 7, 2021
 *      Author: mad
 */

#include <chia/settings.h>


size_t g_read_chunk_size = 65536;
size_t g_write_chunk_size = 4096;

int g_read_thread_divider = 4;

namespace phase2 {
  int g_thread_multi = 1;
}
