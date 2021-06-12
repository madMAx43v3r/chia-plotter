#!/bin/bash

mkdir -p build

cd build

cmake -D CMAKE_CXX_FLAGS="-O3 -fmax-errors=1" -DARITH="easy" -DBUILD_BLS_PYTHON_BINDINGS=false ..

make -j8 $@

