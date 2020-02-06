#!/usr/bin/env bash

set -euo pipefail

platformio run -t size

docker buildx build -t test .

docker run --rm -ti -v $(pwd)/test:/test -v $(pwd)/src:/src -v $(pwd)/build:/build test /test/runtest.sh
