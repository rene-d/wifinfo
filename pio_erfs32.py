#!/usr/bin/env python3
# ERFS.cpp - Embedded Read-only File System (ERFS)
# Copyright (c) 2020 René Devichi. All rights reserved.

# Replace the fs builder tool in the SCons environment

Import("env")


env.Replace(
    MKFSTOOL="./mkerfs32.py"
)
