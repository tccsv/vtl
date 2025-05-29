#!/bin/bash

set -e
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
cmake --build build --config Release -j8
./app/VTL