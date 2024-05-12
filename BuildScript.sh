#!/bin/bash

set -e

cd OpenCL-Headers
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=../_deps
cmake --build build --target install

cd ..
zig build test
