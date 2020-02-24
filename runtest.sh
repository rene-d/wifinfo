#!/bin/sh
# module téléinformation client
# rene-d 2020

set -e


if [ -f /.dockerenv ]; then
    src_dir=/tic
    results=/results
    mkdir -p /build /results
    cd /build
else
    cd "$(dirname ""$0"")"
    src_dir=..
    results=.
    mkdir -p build
    cd build
fi

cmake -DCODE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug ${src_dir}
make -j

ctest --output-on-failure

gcovr -r ${src_dir} --html --html-details -o ${results}/coverage.html

cppcheck --project=compile_commands.json --suppress=*:/usr/local/include/nlohmann/json.hpp --xml 2> ${results}/cppcheck.xml

cppcheck-htmlreport --file ${results}/cppcheck.xml --report-dir=${results}/cppcheck
