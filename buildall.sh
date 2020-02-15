#!/usr/bin/env bash

set -euo pipefail

platformio run -t size
platformio run -t buildfs

docker buildx build -t test .
docker run --rm -ti \
    -v $(pwd):/tic:ro \
    -v $(pwd)/build/docker:/build \
    -v $(pwd)/coverage:/coverage \
    test \
    /tic/runtest.sh
