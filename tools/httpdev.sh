#!/usr/bin/env bash

set -euo pipefail

cwd=$(cd $(dirname $0); pwd)

docker run -ti --rm \
    -e TINFO_MODULE=${1:-192.168.1.100:80} \
    -v $cwd/../data_src:/usr/share/nginx/html \
    -v $cwd/default.conf:/tmp/default.conf \
    -p 5001:80 --name nginx \
    nginx \
    sh -c "envsubst '\${TINFO_MODULE}' < /tmp/default.conf > /etc/nginx/conf.d/default.conf; exec nginx -g 'daemon off;'"