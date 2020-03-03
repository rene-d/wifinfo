#!/usr/bin/env python3
# module téléinformation client
# rene-d 2020

from pathlib import Path
import re
import shutil
import tempfile
import zipfile

import click


class Amalgamation:
    """
    Concatenate all source files into one.
    """

    def __init__(self, src: Path):
        self.src = src
        self.lines = []
        self.included = set()
        # self.system_headers = set()

        self.lines.append(f"//amalgamation: {src} *.ino *.cpp\n")

        for file in src.glob("*.ino"):
            self._add(file)

        f = Path(src / "main.cpp")
        if f.is_file():
            self._add(f)

        for file in src.glob("*.cpp"):
            self._add(file)

    def _add(self, file: Path):

        rel = file.relative_to(self.src)
        if rel in self.included:
            return

        self.included.add(rel)

        self.lines.append("\n")
        self.lines.append(f"//begin: {rel}\n")

        for line in file.open():
            if line.lstrip().startswith("#pragma once"):
                line = "// " + line

            elif line.lstrip().startswith("#include"):
                m = re.search(r'#include "(.+)"', line)
                if m:
                    h = file.parent / m.group(1)

                    line = "// " + line
                    if h.relative_to(self.src) not in self.included:
                        self.lines.append(line)
                        self._add(h)
                        continue

                else:

                    m = re.search(r"#include <(.+)>", line)
                    if m:
                        h = m.group(1)
                        # if h in self.system_headers:
                        #     line = "// " + line
                        # else:
                        #     self.system_headers.add(h)

                    else:

                        print("error:", line)
                        exit(2)

            self.lines.append(line)

        self.lines.append(f"//end: {rel}\n")


@click.command()
@click.argument("target")
@click.option("-c", "--copy", is_flag=True, default=False)
@click.option("-1", "--one", is_flag=True, default=False)
def cmd_arduino_ide(target: str, copy: bool, one: bool) -> None:
    """
    Crée une archive pour utilisation avec l'IDE Arduino.
    """

    target = Path(target)
    root = Path("wifinfo")
    src = Path("src")
    data = Path("data")

    if copy:

        def cp(src: Path, dst: Path) -> None:
            dst = target / dst
            dst.parent.mkdir(exist_ok=True, parents=True)
            shutil.copy2(src, dst)

        op = cp
    else:
        if target.suffix == "":
            target = target.with_suffix(".zip")
        z = zipfile.ZipFile(target, "w")
        op = z.write

    if one:
        ino = tempfile.NamedTemporaryFile("w+")
        ino.writelines(Amalgamation(src).lines)
        ino.flush()

        d = root / "wifinfo.ino"
        print("*", "->", d)

        op(ino.name, d)
    else:

        # ajout de src/* => wifinfo/*
        for f in src.glob("*"):
            if f.is_file():
                d = root / f.relative_to(src)
                print(f, "->", d)
                op(f, d)

    # ajout de data/* => wifinfo/data/*
    for f in data.rglob("*"):
        if f.is_file():
            d = root / f
            print(f, "->", d)
            op(f, d)


if __name__ == "__main__":
    cmd_arduino_ide()
