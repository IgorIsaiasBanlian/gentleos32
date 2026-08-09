#!/usr/bin/env python3
#
# Copyright (c) 2026 luke8086
# Distributed under the terms of GPL-2 License.
#
# File: padfile.py - Pad a file with zeros to a minimum size
#

import argparse
import os


def pad_file(path, size):
    current = os.path.getsize(path)

    if current >= size:
        print(f"{path}: already {current}B, skipped padding")
        return

    with open(path, "ab") as f:
        f.write(b"\0" * (size - current))

    print(f"{path}: padded from {current}B to {size}B")


def main():
    parser = argparse.ArgumentParser(description="Pad a file with zeros to a minimum size")
    parser.add_argument("path", help="file to pad")
    parser.add_argument("size", type=int, help="minimum size in bytes")
    args = parser.parse_args()

    pad_file(args.path, args.size)


main()
