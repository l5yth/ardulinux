## Building the example

### Requirements

1. A system running Linux.
2. GCC (`sudo apt-get install build-essential` or equivalent).
3. PlatformIO — see https://platformio.org/install for instructions.

Hardware support is gated on libgpiod: if it is present, Linux GPIO and I2C are compiled in automatically (this also requires libi2c — `libi2c-dev` on Debian/Ubuntu, `i2c-tools` on Arch). Without libgpiod the build uses simulated devices.

### Build

```sh
cd example
pio run
```

The `platformio.ini` uses `platform = symlink://../` which points at the ArduLinux platform root — no separate install required when working inside this repository. Initialise the platform's submodules first (`git submodule update --init --recursive` from the repo root), otherwise the core sources are missing and the build fails.

To consume ArduLinux from a project outside this repository, replace the `platform` line with the git URL:

```ini
platform = git+https://github.com/l5yth/ardulinux.git
```
