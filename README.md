# GentleOS/32

A hobby operating system for vintage 32-bit PCs.

Its goal is to provide a simple platform for tinkering with retro
hardware and running graphical interactive apps on bare metal.

At minimum, it only requires an i386 CPU, 2MB of RAM, and a VGA display
capable of 640x480x16 mode.

By design it's entirely monolithic and only supports standard
PC devices: VGA/SVGA, keyboard, PS/2 mouse, serial mouse, PC speaker.
The only future plans are refactoring, bugfixes, optimizations,
and adding more apps.

GentleOS/32 has a pure 16-bit spin-off called
[GentleOS/16](https://github.com/luke8086/gentleos),
which targets devices as old as 80186.

<img src="doc/machimg/t1900c.webp" width="400">

## Downloads

* [gentleos32-disk.img](https://github.com/luke8086/gentleos32/releases/download/latest-release/gentleos32-disk.img) -
  HDD image with GRUB 2, requires 4MB RAM
* [gentleos32-floppy.img](https://github.com/luke8086/gentleos32/releases/download/latest-release/gentleos32-floppy.img) -
  floppy image with GRUB Legacy, requires 2MB RAM, only supports 640x480x16 mode
* [gentleos32.elf](https://github.com/luke8086/gentleos32/releases/download/latest-release/gentleos32.elf) -
  kernel file that can be booted with either GRUB 2 and GRUB Legacy

## Running

For a quick test, install QEMU, fetch the HDD image, and run:

```bash
qemu-system-i386 -drive format=raw,file=gentleos32-disk.img -m 8 -debugcon stdio
```

For details, see [USAGE.md](USAGE.md).

## Gallery
<img src="doc/machimg/380z.webp" width="400"> <img src="doc/machimg/t1800.webp" width="400">
<img src="doc/machimg/libr20.webp" width="400"> <img src="doc/machimg/380z-2.webp" width="400">

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
