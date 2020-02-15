#!/bin/sh
# module téléinformation client
# rene-d 2020

if [ -f /.dockerenv ]; then
    src_dir=/tic
    cov_dir=/coverage
else
    src_dir=$(dirname $0)
    cov_dir=./coverage
fi

cmake -DCODE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug ${src_dir}
make -j

lcov --base-directory . --directory . --zerocounters -q

ctest --output-on-failure

lcov --base-directory . --directory . -c -o tic.info
lcov --remove tic.info "/usr*" -o tic.info
genhtml -o ${cov_dir} -t "tic test coverage" --num-spaces 4 tic.info
