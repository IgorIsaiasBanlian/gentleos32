# GentleOS/32

A hobby operating system for vintage 32-bit PCs,
built for tinkering with old hardware on the bare metal.

You can find more information on its [website](https://luke8086.dev/gentleos32).

It has a spin-off called
[GentleOS/16](https://github.com/luke8086/gentleos),
which targets even older, 16-bit PCs.

<img src="doc/machimg/t1900c.webp" width="400">

## Building

The only prerequisite is Docker & Docker Compose, supporting linux/amd64 platform.

To compile GentleOS/32, run:

```bash
docker compose run --rm dev make -j4
```

You will find the resulting binaries in `build/`.

To clean up docker artifacts, run:

```bash
docker compose down --rmi all
```

## Setting wallpaper

A wallpaper can be provided using an initial RAM disk (initrd).
To create it, you need Python 3 with [Pillow](https://pillow.readthedocs.io/),
and [mtools](https://www.gnu.org/software/mtools/).
On macOS:

```bash
brew install pillow mtools
```

To create the initrd and install it in the disk image, run:

```bash
./tools/mkinitrd.py --wallpaper /path/to/image.png --disk-image gentleos32-disk.img
```

Note: the wallpaper will only be shown in 256-color video modes.
In GIMP you can import the provided [VGA palette](misc/vga-256.gpl)
and use indexed mode for best results.

## Attributions

- Assets in [vendor/icons8](vendor/icons8) have been sourced from
  [Icons8](https://icons8.com/) using the
  [free license](https://web.archive.org/web/20260325111643/https://icons8.com/license)
  and modified

- Assets in [vendor/mona](vendor/mona) have been extracted from the
  [Mona Font](https://github.com/MonadABXY/mona-font) and modified
  ([LICENSE](vendor/mona/LICENSE.txt))

- Assets in [vendor/int10h](vendor/int10h) have been extracted from the
  [The Ultimate Oldschool PC Font Pack](https://int10h.org/oldschool-pc-fonts/)
  and modified ([LICENSE](vendor/int10h/LICENSE.txt))

## License

Except where otherwise noted, GentleOS/32 is licensed under [GPLv2](LICENSE).
