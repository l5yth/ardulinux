# ArduLinux

ArduLinux is a continuation of [portduino](https://github.com/geeksville/framework-portduino) and implements the Arduino API as a Linux user-space library, allowing firmware written for embedded targets (nRF52, ESP32, AVR, etc.) to build and run on Linux without modification. Applications get access to real hardware - GPIO via libgpiod, SPI via spidev, I2C via i2c-dev, Serial via POSIX file descriptors - or fully simulated devices for CI and development.

## Differences from portduino

ArduLinux is a clean-room continuation, not a fork. The key differences:

- **No vendored dependencies** — [ArduinoCore-API](https://github.com/arduino/ArduinoCore-API) and [WiFi](https://github.com/arduino-libraries/WiFi) are upstream git submodules, not copied or patched source trees. Updates are a `git submodule update` away.
- **PlatformIO via upstream platform-native** — no custom platform registry entry or private package mirror required.
- **Smaller surface area** — dead code, unused variants, and IDE project files removed. What remains is what runs.

## Requirements

- GCC or Clang with C++14 support
- CMake 3.17+
- libgpiod 1.x or 2.x
- libi2c
- pkg-config

On Debian/Ubuntu:
```sh
sudo apt-get install build-essential cmake libgpiod-dev libi2c-dev pkg-config
```

On Arch:
```sh
sudo pacman -S base-devel cmake libgpiod i2c-tools pkg-config
```

## Build

```sh
cmake -B build
cmake --build build
```

When libgpiod is detected, Linux hardware GPIO and I2C support is compiled in automatically. Without it, the build falls back to simulated GPIO and I2C only.

## Using as a framework (PlatformIO)

Add to your `platformio.ini`:

```ini
[env:native]
platform = https://github.com/platformio/platform-native
framework = arduino
board = linux_hardware
build_flags = ${env.build_flags} -O0 -lgpiod -li2c
```

## Writing an application

Implement the standard Arduino `setup()` and `loop()` functions. Optionally define `ardulinuxSetup()` to bind real hardware before `setup()` runs:

```cpp
#include "Arduino.h"
#include "linux/gpio/LinuxGPIOPin.h"

void ardulinuxSetup() {
    // Bind pin 7 to GPIO chip "gpiochip0", line "PIN_7"
    gpioBind(new LinuxGPIOPin(7, "gpiochip0", "PIN_7"));
}

void setup() {
    pinMode(7, OUTPUT);
}

void loop() {
    digitalWrite(7, !digitalRead(7));
    delay(500);
}
```

Without `ardulinuxSetup()`, all pins default to simulated. The VFS is mounted at `~/.ardulinux/default/` by default; pass `--fsdir <path>` to override or `--erase` to wipe it on startup.

## License

LGPL 2.1 - see [LICENSE](LICENSE).

* Copyright (c) 2011-19 Arduino LLC.
* Copyright (c) 2020-23 Geeksville Industries, LLC
* Copyright (c) 2024-26 jp-bennett
* Copyright (c) 2026-27 l5yth
