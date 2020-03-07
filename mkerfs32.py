#!/usr/bin/env python3
# ERFS.cpp - Embedded Read-only File System (ERFS)
# Copyright (c) 2020 René Devichi. All rights reserved.

#
# ERFS is heavily inspired by Microchip Proprietary File System (MPFS2)
# It is intended to provide a small read-only filesystem for ESP8266.
#
# ERFS Structure:
#     [E][P][F][S]
#     [BYTE Ver Hi][BYTE Ver Lo][WORD Number of Files]
#     [Name Hash 0][Name Hash 1]...[Name Hash N]
#     [File Record 0][File Record 1]...[File Record N]
#     [String 0][String 1]...[String N]
#     [File Data 0][File Data 1]...[File Data N]
#
# Name Hash (2 bytes):
#     hash = 0
#     for each(byte in name)
#         hash += byte
#         hash <<= 1
#
#     Technically this means the hash only includes the
#     final 15 characters of a name.
#
# File Record Structure (16 bytes):
#     [DWORD String Ptr]
#     [DWORD Data Ptr]
#     [DWORD Len]
#     [DWORD Timestamp]
#
#     Pointers are absolute addresses within the ERFS image.
#     Timestamp is the UNIX timestamp
#
# String Structure (1 to 64 bytes):
#     ["path/to/file.ext"][0x00]
#
#      All characteres are allowed, / has no special meaning.
#
# File Data Structure (arbitrary length):
#     [File Data]
#
#
# All values are aligned on 4-byte boundaries, eventually padded with 0x00.
#

import click
from collections import namedtuple
from datetime import datetime
from fnmatch import fnmatch
import os
from pathlib import Path
import struct
import sys
import time


HEADER_SIZE = 8
FATRECORD_SIZE = 16

FATRecord = namedtuple("FATRecord", ["StringPtr", "DataPtr", "Len", "Timestamp"])


def get_string(fs: bytes, ptr: int, hex_name=False) -> str:
    if fs[ptr] == 0:
        if hex_name:
            return f"{ptr:06X}"
        else:
            return "<no name>"
    s = ""
    for i in fs[ptr:]:
        if i == 0:
            break
        elif i < 32:
            s += f"<{i}>"
        else:
            s += chr(i)
    return s


def align32(i: int) -> int:
    if i & 3 == 0:
        return i
    else:
        return (i | 3) + 1


def create(data_dir: str, size: str, image_file: str) -> None:
    """
    Create a ERFS filesystem image.
    """

    if size[-1].lower() == "k":
        size = int(size[:-1]) * 1024
    elif size[-1].lower() == "m":
        size = int(size[:-1]) * 1048576
    else:
        size = int(size)

    click.echo(click.style("ERFS 3.2 builder", fg="bright_green"))

    total_size = HEADER_SIZE  # the header
    files = []
    for f in Path(data_dir).rglob("*"):
        if f.is_dir():
            continue

        if f.name in [".DS_Store", ".git"]:
            continue

        name = f.relative_to(data_dir).as_posix().encode()

        total_size += 2 + FATRECORD_SIZE + align32(len(name) + 1) + align32(f.stat().st_size)
        files.append((f, name, f.stat()))

        click.echo(click.style(f"{f.stat().st_size:>10} {name.decode()}", fg="bright_black"))

    total_size = align32(total_size)  # need to align because of the hash table

    if total_size > size:
        print(f"FS too small, missing {total_size - size} bytes", file=sys.stderr)
        exit(2)

    num_files = len(files)

    print(f"FS ok, {total_size} bytes occupied, {size - total_size} bytes free, {num_files} files")

    fs = Path(image_file).open("wb")

    # 8 byte header
    fs.write(b"ERFS\x03\x02")
    fs.write(struct.pack("<H", num_files))

    # hash table: 2 * num_files bytes
    for _, name, _ in files:
        hash = 0
        for c in name:
            hash += c
            hash *= 2
        fs.write(struct.pack("<H", hash & 0xFFFF))

    # align on 32-bit boundary
    if num_files & 1 != 0:
        fs.write(struct.pack("<H", 0))

    # filename table address (after header+hash table+fat)
    names_ptr = HEADER_SIZE + align32(2 * num_files) + FATRECORD_SIZE * num_files

    # filename table expected length
    names_len = sum(align32(len(name) + 1) for _, name, _ in files)

    # storage area
    data_ptr = names_ptr + names_len

    # FAT: 16 * num_files bytes
    for _, name, st in files:
        fs.write(struct.pack("<IIII", names_ptr, data_ptr, st.st_size, int(st.st_mtime)))
        names_ptr += align32(len(name) + 1)
        data_ptr += align32(st.st_size)

    # file names
    for _, name, _ in files:
        fs.write(name)
        # ending \0 and word alignment
        padding = align32(len(name) + 1) - len(name)
        fs.write(b"\0" * padding)

    # file contents
    for f, _, st in files:
        fs.write(f.read_bytes())
        padding = align32(st.st_size) - st.st_size
        fs.write(b"\0" * padding)

    print(f"wrote {fs.tell()} bytes so far")

    fs.truncate(size)
    fs.close()

    print(f"final size is {size} bytes")
    print(f"{image_file} written")


def list_content(image_file: str, verbose: bool, extract_dir: str, patterns) -> None:
    """
    List or extract the content of a ERFS image.
    """

    fs = Path(image_file).read_bytes()

    # process filesystem header
    signature, ver_hi, ver_lo, n = struct.unpack("<4sBBH", fs[0:HEADER_SIZE])
    if signature != b"ERFS":
        print("File is not a ERFS filesystem", file=sys.stderr)
        exit(2)

    offset = HEADER_SIZE
    name_hash = [0] * n
    for i in range(n):
        (name_hash[i],) = struct.unpack("<H", fs[offset : offset + 2])
        offset += 2
    offset = align32(offset)

    record = [None] * n
    for i in range(n):
        record[i] = FATRecord._make(struct.unpack("<IIII", fs[offset : offset + FATRECORD_SIZE]))
        offset += FATRECORD_SIZE

    if extract_dir:
        if extract_dir != "-":
            extract_dir = Path(extract_dir)
    else:
        print(f"Version: {ver_hi}.{ver_lo}")
        print(f"Number of files: {n}")

    for i, r in enumerate(record, 1):

        if fs[r.StringPtr] == 0:
            print(f"Bad file entry {i}: should have a name", i, file=sys.stderr)
            break

        filename = get_string(fs, r.StringPtr)

        if extract_dir:

            if patterns:
                if not any(fnmatch(filename, pattern) for pattern in patterns):
                    continue

            if extract_dir == "-":
                print(f"extracted {i}/{n}: {filename} {r.Len} bytes", file=sys.stderr)
                sys.stdout.buffer.write(fs[r.DataPtr : r.DataPtr + r.Len])
            else:
                f = extract_dir / filename
                f.parent.mkdir(exist_ok=True, parents=True)

                f.write_bytes(fs[r.DataPtr : r.DataPtr + r.Len])

                timestamp = datetime.fromtimestamp(r.Timestamp)
                mt = time.mktime(timestamp.timetuple())
                os.utime(f.as_posix(), (mt, mt))

                print(f"extracted {i}/{n}: {f.as_posix()} {r.Len} bytes")

        else:
            timestamp = datetime.fromtimestamp(r.Timestamp).strftime("%Y-%m-%dT%H:%M:%SZ")

            if verbose:
                print()
                print(f"FATRecord {i}:")
                print(f"    .StringPtr = 0x{r.StringPtr:06x}  {filename}")
                print(f"    .DataPtr   = 0x{r.DataPtr:06x}")
                print(f"    .Len       = 0x{r.Len:06x}  {r.Len}")
                print(f"    .Timestamp =", r.Timestamp, timestamp)

            else:
                print(f"{i:4d}  {r.Len:8d}  {timestamp}  {filename}")


@click.command(context_settings={"help_option_names": ["-h", "--help"]})
@click.option(
    "-c",
    "--create",
    "data_dir",
    metavar="DATA_DIR",
    help="create image from a directory",
    type=str,
    default="data",
    show_default=True,
)
@click.option("-l", "--list", "list_files", help="list the content of an image", is_flag=True)
@click.option("-x", "--extract", "extract_dir", metavar="DIR", help="extract files to directory", type=str)
@click.option("-s", "--size", metavar="SIZE", help="fs image size, in bytes", type=str, default="1m", show_default=True)
@click.option("-b", "--block", help="ignored", type=int, expose_value=False)
@click.option("-p", "--page", help="ignored", type=int, expose_value=False)
@click.option("-v", "--verbose", help="verbose list", is_flag=True)
@click.argument("image_file")
@click.argument("files", metavar="[FILES_TO_EXTRACT]", nargs=-1)
def main(data_dir, list_files, extract_dir, size, verbose, image_file, files):

    if list_files or extract_dir:
        list_content(image_file, verbose, extract_dir, files)

    else:
        create(data_dir, size, image_file)


if __name__ == "__main__":
    main()
