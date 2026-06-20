# GentleOS/32 - Usage and development

## Pre-built images

For a quick test, you can find pre-built images of GentleOS in
[Releases](https://github.com/luke8086/gentleos32/releases).

## Building

The only prerequisite is Docker & Docker Compose, supporting linux/amd64 platform.

First, copy `config.sample.h` to `config.h` and review your settings. Then run:

```bash
docker compose run --rm dev make -j4
```

To clean up docker artifacts, you can run:

```bash
docker compose down --rmi all
```

## Running in QEMU

Run:

```bash
qemu-system-i386 -drive format=raw,file=build/disk.img -m 8 -debugcon stdio
```

For audio support on Macs, also add:

```bash
-audiodev coreaudio,id=snd0 -machine pcspk-audiodev=snd0
```

## Running on real devices

> [!WARNING]
> The author takes no responsibility for any lost data
> or damaged hardware. Proceed at your own risk, and only if you
> fully understand what you're doing.

### Booting with GRUB

If you already have GRUB installed on the target machine,
you can have it boot `build/gentleos.elf` directly. See
[misc/grub.sample.cfg](misc/grub.sample.cfg) for a sample config.

### Booting from HDD/pendrive

To prepare a bootable HDD/pendrive, run:

```bash
dd if=build/disk.img of=<TARGET DISK> bs=1M conv=fsync status=progress
```

> [!WARNING]
> Note this command will **permanently destroy** data on the target disk,
> and there will be no confirmation prompt.

### Booting from a floppy disk

To prepare a bootable floppy, run:

```bash
dd if=build/floppy.img of=<TARGET DISK> bs=32k conv=fsync status=progress
```

> [!WARNING]
> Note this command will **permanently destroy** data on the target disk,
> and there will be no confirmation prompt.

On subsequent runs you don't need to rewrite the entire image,
it's enough to copy `build/gentleos.elf`.
