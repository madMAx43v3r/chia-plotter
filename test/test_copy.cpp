/*
 * test_copy.cpp
 *
 *  Created on: Jun 8, 2021
 *      Author: mad
 */

#include <chia/copy.h>

#include <iostream>


int main(int argc, char** argv)
{
	if(argc < 3) {
		return -1;
	}
	
	const auto bytes = final_copy(argv[1], argv[2]);
	
	std::cout << bytes << " bytes copied" <<  std::endl;
	
	return 0;
}
