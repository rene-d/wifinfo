#!/usr/bin/env python3

from sseclient import SSEClient

messages = SSEClient("http://192.168.1.96/tic")
for msg in messages:
    print(msg)
