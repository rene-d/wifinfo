#!/usr/bin/env python3
# ERFS.cpp - Embedded Read-only File System (ERFS)
# Copyright (c) 2020 René Devichi. All rights reserved.

#
# ERFS is heavily inspired by Microchip Proprietary File System (MPFS2)
#
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
from pathlib import Path
import struct

HEADER_SIZE = 8
FATRECORD_SIZE = 16


def align32(i):
    if i & 3 == 0:
        return i
    else:
        return (i | 3) + 1


@click.command()
@click.option("-s", "--size", help="fs image size, in bytes", type=str, default="1m", show_default=True)
@click.option("-c", "--create", help="create image from a directory", type=str, default="data", show_default=True)
@click.option("-b", "--block", help="ignored", type=int, expose_value=False)
@click.option("-p", "--page", help="ignored", type=int, expose_value=False)
@click.argument("image_file")
def main(size, create, image_file):

    if size[-1].lower() == "k":
        size = int(size[:-1]) * 1024
    elif size[-1].lower() == "m":
        size = int(size[:-1]) * 1048576
    else:
        size = int(size)

    click.echo(click.style("ERFS 3.2 builder", fg="bright_green"))

    total_size = HEADER_SIZE  # the header
    files = []
    for f in Path(create).rglob("*"):
        if f.is_dir():
            continue

        if f.name in [".DS_Store", ".git"]:
            continue

        name = f.relative_to(create).as_posix().encode()

        total_size += 2 + FATRECORD_SIZE + align32(len(name) + 1) + align32(f.stat().st_size)
        files.append((f, name, f.stat()))

        click.echo(click.style(f"{f.stat().st_size:>10} {name.decode()}", fg="bright_black"))

    total_size = align32(total_size)  # need to align because of the hash table

    if total_size > size:
        print(f"FS too small, missing {total_size - size} bytes")
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


if __name__ == "__main__":
    main()
