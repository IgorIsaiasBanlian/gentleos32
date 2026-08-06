#!/usr/bin/env python3
#
# Copyright (c) 2026 luke8086
# Distributed under the terms of GPL-2 License.
#
# File: mkinitrd.py - Create initial RAM disk
#

# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "pillow>=10,<12",
# ]
# ///

import argparse
import os
import re
import shutil
import struct

MAGIC        = b"IRD1"
NAME_LEN     = 23
ALIGN        = 4
HEADER_LEN   = 8                   # 4s magic + I count
ENTRY_LEN    = NAME_LEN + 9        # 23s name + B type + I offset + I size

FILE_TYPE_UNKNOWN   = 0
FILE_TYPE_BITMAP    = 1

FILE_TYPE_NAMES = {
    FILE_TYPE_UNKNOWN: "unknown",
    FILE_TYPE_BITMAP:  "bitmap",
}

PALETTE_PATH = "misc/vga-256.gpl"
PALETTE_REX  = re.compile(r"^\s*(\d+)\s+(\d+)\s+(\d+)\s+\$([0-9a-fA-F]+)\s*$")

INITRD_PATH  = "gentleos.rd"

SECTOR_LEN    = 512
FS_OFFSET     = 1048576


def die(msg):
    raise SystemExit(msg)


def align(n):
    return (n + ALIGN - 1) & ~(ALIGN - 1)


def split_ext(path):
    [base, ext] = os.path.splitext(os.path.basename(path))
    ext = ext.lower()[1:]
    return [base, ext]


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


def process_image(path):
    try:
        from PIL import Image
    except ImportError:
        die("Error: mkinitrd.py requires 'pillow' library for Python")

    palette = getattr(process_image, "palette", None)

    if palette is None:
        palette = load_palette(PALETTE_PATH)
        process_image.palette = palette

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


def load_file(path):
    [basename, ext] = split_ext(path)

    if ext in  ["jpg", "jpeg", "png", "ppm", "gif", "bmp"]:
        file_type = FILE_TYPE_BITMAP
        data = process_image(path)
    else:
        file_type = FILE_TYPE_UNKNOWN
        with open(path, "rb") as f:
            data = f.read()

    return {
        "name": basename[:NAME_LEN - 1],
        "type": file_type,
        "data": data,
    }


def build_initrd(files):
    count = len(files)
    offset = align(HEADER_LEN + count * ENTRY_LEN)
    table = b""
    blobs = b""

    for f in files:
        size = len(f["data"])
        ftype = f["type"]
        padded_size = align(size)

        print("- %s: %x (%u B, %s)" % (f["name"], offset, size, FILE_TYPE_NAMES[ftype]))

        name = f["name"].encode("latin-1")[:NAME_LEN - 1]
        table += struct.pack("<%dsBII" % NAME_LEN, name, ftype, offset, size)
        blobs += f["data"] + b"\0" * (padded_size - size)
        offset += padded_size

    return struct.pack("<4sI", MAGIC, count) + table + blobs


def get_kernel_offset_in_image(image):
    stage2_sectors_ofs = 5
    stage2_sectors, = struct.unpack_from("<H", image, stage2_sectors_ofs)

    return SECTOR_LEN * (2 + stage2_sectors)


def is_native_image(image):
    if len(image) < SECTOR_LEN:
        return False

    kernel_offset = get_kernel_offset_in_image(image)

    if len(image) < kernel_offset + 32:
        return False

    magic, = struct.unpack_from("<I", image, kernel_offset + 4)

    return magic == 0x1badb002


def install_initrd_native(disk_image_path, image, initrd):
    kernel_offset = get_kernel_offset_in_image(image)
    kernel_sectors, = struct.unpack_from("<H", image, SECTOR_LEN * 2)
    kernel_end = kernel_offset + kernel_sectors * SECTOR_LEN

    if kernel_sectors == 0 or len(image) < kernel_end:
        die("Error: no kernel found in the disk image")

    initrd_sectors = (len(initrd) + SECTOR_LEN - 1) // SECTOR_LEN

    if initrd_sectors * SECTOR_LEN > 15 * 1024 * 1024:
        die("Error: initrd too big to fit in 15MB of RAM")

    padding = b"\0" * (initrd_sectors * SECTOR_LEN - len(initrd))
    image = bytearray(image[:kernel_end]) + initrd + padding

    initrd_sectors_offset = SECTOR_LEN * 2 + 2
    struct.pack_into("<H", image, initrd_sectors_offset, initrd_sectors)

    with open(disk_image_path, "wb") as f:
        f.write(image)

    print("Initrd installed in %s" % disk_image_path)


def install_initrd_grub(disk_image_path):
    if not shutil.which("mcopy"):
        die("Error: mkinitrd.py requires 'mtools' package to install initrd in a disk image")

    cmd = "mcopy -D o -i '%s@@%d' %s ::" % (disk_image_path, FS_OFFSET, INITRD_PATH)
    print("Running %s" % cmd)
    os.system(cmd)


def install_initrd(disk_image_path, initrd):
    if not os.path.exists(disk_image_path):
        die("Error: disk image not found")

    with open(disk_image_path, "rb") as f:
        image = f.read()

    if is_native_image(image):
        install_initrd_native(disk_image_path, image, initrd)
    else:
        install_initrd_grub(disk_image_path)


def main():
    parser = argparse.ArgumentParser(
        description="Create initial RAM disk for GentleOS/32 in %s" % INITRD_PATH,
    )
    parser.add_argument("files", nargs="*", help="files to add")
    parser.add_argument("--wallpaper", metavar="PATH", help="image to use as the wallpaper")
    parser.add_argument("--disk-image", metavar="PATH", help="disk image to install initrd into")
    args = parser.parse_args()

    files = []

    for path in args.files:
        files.append(load_file(path))

    if args.wallpaper is not None:
        data = process_image(args.wallpaper)
        files.append({
            "name": "wallpaper",
            "type": FILE_TYPE_BITMAP,
            "data": data,
        })

    if not files:
        parser.print_usage()
        raise SystemExit(1)

    print("Generating initrd:")
    image = build_initrd(files)

    with open(INITRD_PATH, "wb") as f:
        f.write(image)

    print("Initrd saved to %s" % INITRD_PATH)

    if args.disk_image is not None:
        install_initrd(args.disk_image, image)


if __name__ == "__main__":
    main()
