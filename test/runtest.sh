#!/bin/sh

cmake -DCODE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug /test
make -j

lcov --base-directory . --directory . --zerocounters -q

ctest --output-on-failure

lcov --base-directory . --directory . -c -o tic.info
lcov --remove tic.info "/usr*" -o tic.info
genhtml -o /test/coverage -t "tic test coverage" --num-spaces 4 tic.info
