#!/usr/bin/env python3

import zipfile
from pathlib import Path
import click


@click.command()
@click.argument("archive")
def cmd_arduino_ide(archive):
    """
    Crée une archive pour utilisation avec l'IDE Arduino.
    """
    z = zipfile.ZipFile(archive, "w")
    root = Path("wifinfo")
    src = Path("src")
    data = Path("data")

    # ajout de src/* => wifinfo/*
    for f in src.glob("*"):
        d = root / f.relative_to(src)
        print(d)
        z.write(f.as_posix(), d.as_posix())

    # ajout de data/* => wifinfo/data/*
    for f in data.rglob("*"):
        d = root / f
        print(d)
        z.write(f.as_posix(), d.as_posix())


if __name__ == "__main__":
    cmd_arduino_ide()
