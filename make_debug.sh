#!/bin/bash

set -euxo pipefail

mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=Debug -DARITH="easy" -DBUILD_BLS_PYTHON_BINDINGS=false ..
cmake --build . --config Debug --parallel 8 -- $@
