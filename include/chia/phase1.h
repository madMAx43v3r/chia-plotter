/*
 * phase1.h
 *
 *  Created on: May 25, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_PHASE1_H_
#define INCLUDE_CHIA_PHASE1_H_

#include <chia/entries.h>

#include <vector>


namespace phase1 {

struct input_t {
	
};

struct ouput_t {
	
};

template<typename T>
struct match_t {
	T left;
	T right;
	uint32_t pos = 0;
	uint16_t off = 0;
};


} // phase1

#endif /* INCLUDE_CHIA_PHASE1_H_ */
