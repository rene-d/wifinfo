#!/usr/bin/env bash
# module téléinformation client
# rene-d 2020

set -euo pipefail

cwd="$(cd $(dirname ""$0""); pwd)"
echo $cwd

docker run -ti --rm \
    -e "TINFO_MODULE=${1:-192.168.4.1:80}" \
    -v "$cwd/../data_src:/usr/share/nginx/html" \
    -v "$cwd/default.conf:/tmp/default.conf" \
    -p 5001:80 --name nginx \
    nginx \
    sh -c "envsubst '\${TINFO_MODULE}' < /tmp/default.conf > /etc/nginx/conf.d/default.conf; exec nginx -g 'daemon off;'"
