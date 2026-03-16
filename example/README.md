## Building the example

### Requirements

1. A system running Linux.
2. GCC (`sudo apt-get install build-essential` or equivalent).
3. PlatformIO — see https://platformio.org/install for instructions.

libgpiod (`libgpiod-dev`) is optional. If present, Linux hardware GPIO and I2C are compiled in automatically. Without it the build falls back to simulated devices.

### Build

```sh
cd example
pio run
```

The `platformio.ini` uses `platform = symlink://../` which points at the ArduLinux platform root — no separate install required when working inside this repository.

To consume ArduLinux from a project outside this repository, replace the `platform` line with the git URL:

```ini
platform = git+https://github.com/l5yth/ardulinux.git
```
