#!/usr/bin/env bash

set -euo pipefail

platformio run -t size

docker buildx build -t test .

docker run --rm -ti \
    -v $(pwd)/test:/test \
    -v $(pwd)/src:/test/src \
    -v $(pwd)/build:/build \
    -v $(pwd)/coverage:/coverage \
    test /test/runtest.sh
