#!/bin/bash

cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O3 -fmax-errors=1" .
cd build
make -j8 $@
