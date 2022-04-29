#!/bin/bash

set -euxo pipefail

mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=Release -DARITH="easy" -DBUILD_BLS_PYTHON_BINDINGS=false -DBUILD_BLS_TESTS=false -DBUILD_BLS_BENCHMARKS=false ..
cmake --build . --config Release --parallel 8 -- $@
