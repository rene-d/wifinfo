#!/usr/bin/env python3
# module téléinformation client
# rene-d 2020

from sseclient import SSEClient
import click
import socket
import requests
import time


@click.command(context_settings={"help_option_names": ["-h", "--help"]})
@click.argument("server", default="192.168.4.1")
def main(server):

    while True:
        try:
            messages = SSEClient(f"http://{server}/tic", timeout=10)
            for msg in messages:
                print(msg)

        except (socket.timeout, requests.exceptions.ConnectionError) as exception:
            print(exception)
            for i in range(10, 0, -1):
                print(f"retry in {i}s")
                time.sleep(1)


if __name__ == "__main__":
    main()
