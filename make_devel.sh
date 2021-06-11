#!/bin/bash

mkdir -p build

cd build

cmake -D CMAKE_CXX_FLAGS="-g -O3 -fmax-errors=1" -DARITH="easy" -DBUILD_BLS_PYTHON_BINDINGS=false -DBUILD_BLS_TESTS=false -DBUILD_BLS_BENCHMARKS=false ..

make -j8 $@

