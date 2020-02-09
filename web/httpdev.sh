#!/bin/sh

docker run -ti --rm -v $PWD/../data_src:/usr/share/nginx/html -v $PWD/default.conf://etc/nginx/conf.d/default.conf -p 5001:80 --name nginx nginx
