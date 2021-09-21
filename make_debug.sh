#!/bin/bash

cmake -B build -DCMAKE_BUILD_TYPE=Debug .
cd build
make -j8 $@
