#!/bin/bash

cmake -B build .
cd build
make -j8 $@
