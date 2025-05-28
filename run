#!/bin/bash

set -e
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
cmake --build build --config Release -j8
ctest --test-dir build --output-on-failure
./build/VTL