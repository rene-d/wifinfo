#!/bin/sh

platformio run -t size

mkdir -p test/build
cd test/build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
