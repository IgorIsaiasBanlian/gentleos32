#!/usr/bin/env python3
#
# Copyright (c) 2026 luke8086
# Distributed under the terms of GPL-2 License.
#
# File: mkinitrd.py - Create initial RAM disk
#

import argparse
import os
import re
import shutil
import struct

MAGIC        = b"IRD1"
NAME_LEN     = 24
ALIGN        = 4
HEADER_LEN   = 8                   # 4s magic + I count
ENTRY_LEN    = NAME_LEN + 8        # 24s name + I offset + I size

PALETTE_PATH = "misc/vga-256.gpl"
PALETTE_REX = re.compile(r"^\s*(\d+)\s+(\d+)\s+(\d+)\s+\$([0-9a-fA-F]+)\s*$")
INITRD_PATH = "gentleos.rd"


def die(msg):
    raise SystemExit(msg)


def align(n):
    return (n + ALIGN - 1) & ~(ALIGN - 1)


def load_palette(path):
    rgb = [None] * 256

    with open(path) as f:
        for line in f:
            m = PALETTE_REX.match(line)
            if not m:
                continue
            r, g, b, index = int(m[1]), int(m[2]), int(m[3]), int(m[4], 16)
            if rgb[index] is None:
                rgb[index] = (r, g, b)

    return [c if c is not None else (0, 0, 0) for c in rgb]


def process_wallpaper(path, palette):
    try:
        from PIL import Image
    except ImportError:
        die("Error: mkinitrd.py requires 'pillow' library for Python")

    print("Importing %s... " % path, end="")

    img = Image.open(path).convert("RGB")

    width, height = img.size

    pal = Image.new("P", (1, 1))
    pal.putpalette([c for color in palette for c in color])
    quantized = img.quantize(palette=pal, dither=Image.Dither.FLOYDSTEINBERG)
    pixels = quantized.tobytes()

    header = struct.pack("<7I", width, height, 8, width, 0, 0xfd, 0)

    print("ok (%dx%d)" % (width, height))

    return header + pixels


def load_inputs(args):
    inputs = []

    for path in args.inputs:
        name = os.path.basename(path)[:NAME_LEN - 1]
        with open(path, "rb") as f:
            data = f.read()
        inputs.append({"name": name, "data": data})

    if args.wallpaper is not None:
        palette = load_palette(PALETTE_PATH)
        data = process_wallpaper(args.wallpaper, palette)
        inputs.append({"name": "wallpaper", "data": data})

    return inputs


def build_initrd(inputs):
    count = len(inputs)
    offset = align(HEADER_LEN + count * ENTRY_LEN)
    table = b""
    blobs = b""

    for inp in inputs:
        size = len(inp["data"])
        padded_size = align(size)

        print("- %s: %x (%u B)" % (inp["name"], offset, size))

        name = inp["name"].encode("latin-1")[:NAME_LEN - 1]
        table += struct.pack("<%dsII" % NAME_LEN, name, offset, size)
        blobs += inp["data"] + b"\0" * (padded_size - size)
        offset += padded_size

    return struct.pack("<4sI", MAGIC, count) + table + blobs


def install_initrd(disk_image):
    if not shutil.which("mcopy"):
        die("Error: mkinitrd.py requires 'mtools' package to install initrd in a disk image")

    if not os.path.exists(disk_image):
        die("Error: disk image not found")

    cmd = "mcopy -D o -i '%s@@1048576' %s ::" % (disk_image, INITRD_PATH)
    print("Running %s" % cmd)
    os.system(cmd)


def main():
    parser = argparse.ArgumentParser(
        description="Create initial RAM disk for GentleOS/32 in %s" % INITRD_PATH,
    )
    parser.add_argument("inputs", nargs="*", help="raw files to add")
    parser.add_argument("--wallpaper", metavar="PATH", help="image to use as the wallpaper")
    parser.add_argument("--disk-image", metavar="PATH", help="disk image to install initrd into")
    args = parser.parse_args()

    if not args.inputs and args.wallpaper is None:
        parser.print_usage()
        raise SystemExit(1)

    inputs = load_inputs(args)

    print("Generating initrd:")
    image = build_initrd(inputs)

    with open(INITRD_PATH, "wb") as f:
        f.write(image)

    print("Initrd saved to %s" % INITRD_PATH)

    if args.disk_image is not None:
        install_initrd(args.disk_image)


if __name__ == "__main__":
    main()
