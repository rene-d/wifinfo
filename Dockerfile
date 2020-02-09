FROM alpine:3.10

RUN apk add --no-cache gcc g++ gdb make cmake musl-dev gtest-dev vim wget curl bash perl \
&&  wget -P /usr/local/include/nlohmann/ https://github.com/nlohmann/json/releases/download/v3.7.3/json.hpp

RUN curl -slkL https://github.com/linux-test-project/lcov/releases/download/v1.14/lcov-1.14.tar.gz | tar -C /tmp -xzf - \
&&  cd /tmp/lcov-1.14 \
&&  make install \
&&  cd / \
&&  rm -rf /tmp/lcov-1.14

VOLUME /coverage
VOLUME /test

WORKDIR /build
