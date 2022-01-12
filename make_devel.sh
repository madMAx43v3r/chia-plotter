#!/bin/bash

set -euxo pipefail

mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DARITH="easy" -DBUILD_BLS_PYTHON_BINDINGS=false ..
cmake --build . --config RelWithDebInfo --parallel 8 -- $@
