# GentleOS/32

A fantasy operating system for vintage 32-bit PCs,
built for tinkering with old hardware on the bare metal.

You can find more information on its [website](https://luke8086.dev/gentleos/32).

It has a pure 16-bit sibling called
[GentleOS/16](https://github.com/luke8086/gentleos).

<img src="https://luke8086.dev/gentleos/images/gentleos32-photo-t1900c-nobg.webp" width="300">

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

## Testing

The easiest way to test is to open `gentleos32-web.html` in a browser.

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

- Assets in [vendor/anthicon](vendor/anthicon) have been obtained from
  [Anthony's Icon Library](https://antofthy.gitlab.io/aicons/)
  created by Anthony Thyssen ([README](vendor/anthicon/README.txt))

- Assets in [vendor/squidfng](vendor/squidfng) have been originally created by
  [Travis Beckham](https://everythingisabox.com/), obtained from
  [Squidfingers](https://web.archive.org/web/20061023062010/http://www.squidfingers.com/)
  and modified ([README](vendor/squidfng/README.txt))

- Assets in [vendor/misc](vendor/misc) have been found online and modified
  ([README](vendor/misc/README.txt))

## License

Except where otherwise noted, GentleOS/32 is licensed under [GPLv2](LICENSE).
