# module téléinformation client
# rene-d 2020

FROM alpine:3.11

RUN apk add --no-cache gcc g++ gdb make cmake musl-dev gtest-dev vim wget curl bash gcovr cppcheck cppcheck-htmlreport \
&&  wget -P /usr/local/include/nlohmann/ https://github.com/nlohmann/json/releases/download/v3.7.3/json.hpp

VOLUME /tic

VOLUME /results

WORKDIR /build
