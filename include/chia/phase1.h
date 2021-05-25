/*
 * phase1.h
 *
 *  Created on: May 25, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_PHASE1_H_
#define INCLUDE_CHIA_PHASE1_H_

#include <chia/entries.h>
#include <chia/Thread.h>

#include <vector>


namespace phase1 {

struct input_t {
	
};

struct ouput_t {
	
};


void compute_f1(const uint8_t* id, int num_threads, Processor<std::vector<entry_1>>* output);


} // phase1

#endif /* INCLUDE_CHIA_PHASE1_H_ */
