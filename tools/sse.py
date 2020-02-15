#!/usr/bin/env python3

from sseclient import SSEClient
import click


@click.command(context_settings={"help_option_names": ["-h", "--help"]})
@click.argument("server", default="192.168.4.1")
def main(server):
    messages = SSEClient(f"http://{server}/tic")
    for msg in messages:
        print(msg)


if __name__ == "__main__":
    main()
