# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

ArduLinux ports the Arduino API to run as user-space applications on Linux. It lets Arduino-compatible code (targeting nrf52, esp32, etc.) run on Linux using real hardware (GPIO via libgpiod, SPI via spidev, I2C via i2c-dev, serial via file descriptors) or fully simulated devices. The primary consumer is Meshcore for CI/CD simulation tests and Linux device targets.

## Build

**CMake** (used for IDE development and direct builds):
```sh
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug
```

**PlatformIO** (used when consuming the framework from an application):
```sh
cd example
pio run
```
The `example/platformio.ini` references `platform-native` from `github.com/meshcore-dev/platform-native.git`.

## Tests

Tests are low-level Linux hardware tests (not framework unit tests) and require root:
```sh
cd tests
gcc -o spidev spidev.cpp && sudo ./spidev   # tests /dev/spidev0.0
gcc -o gpio gpio.cpp && sudo ./gpio         # tests /dev/gpiochip2
```
These test the Linux kernel interfaces directly, not the ArduLinux framework layer.

## Architecture

### Initialization flow

`main()` in `cores/ardulinux/main.cpp` drives everything:

1. `ardulinuxCustomInit()` — weak hook; applications use this to call `ardulinuxAddArguments()` before argp runs
2. argp parses `--erase` / `--fsdir` flags
3. VFS mounted at `~/.ardulinux/default/` (or `--fsdir` path)
4. `gpioInit()` — allocates 64 `SimGPIOPin` instances by default
5. `ardulinuxSetup()` — weak hook; applications call `gpioBind()` here to attach real hardware pins
6. `setup()` — standard Arduino setup (application-provided, required)
7. Main loop: `gpioIdle()` → `loop()` → optional 100 ms sleep when no real hardware

### Hardware substitution model

All GPIO pins start as `SimGPIOPin`. Applications replace specific pins with real hardware by calling `gpioBind(new LinuxGPIOPin(...))` inside `ardulinuxSetup()`. Setting any real pin flips the global `realHardware` flag, which disables the loop sleep.

`LinuxGPIOPin` supports both libgpiod v1 and v2 (detected at compile time via `GPIOD_LINE_BULK_MAX_LINES`; v1 sets `GPIOD_V 1`, v2 sets `GPIOD_V 2`). Constructor accepts either a pin name or numeric offset alongside a chip label.

ISRs are polled, not interrupt-driven: `gpioIdle()` calls `refreshIfNeeded()` on every pin each loop iteration.

### Weak function hooks

Applications implement these to integrate with the framework:

| Function | Purpose |
|---|---|
| `void ardulinuxCustomInit()` | Very early init; add argp children here |
| `void ardulinuxSetup()` | Bind real hardware; runs after arg parsing and VFS mount |
| `void setup()` | Standard Arduino setup (required) |
| `void loop()` | Standard Arduino loop (required) |

### Header/include layers

- `cores/arduino/api/` — upstream ArduinoCore-API (submodule); do not modify
- `cores/ardulinux/Arduino.h` — top-level include for application code; pulls in the Arduino API and platform-specific implementations
- `cores/ardulinux/ArduLinuxGPIO.h` — `GPIOPinIf` / `GPIOPin` / `SimGPIOPin` hierarchy and `gpioBind()`
- `cores/ardulinux/FS/ArduLinuxFS.h` — exposes `ArduLinuxFS` (fs::FS) and `ardulinuxVFS` (shared_ptr<VFSImpl>)
- `cores/ardulinux/Utility.h` — `ardulinuxError()` (throws), `ardulinuxCheckNotNeg()`, `ardulinuxCheckZero()`, `ardulinuxDebug()` (SIGINT)

### Preprocessor define

`ARDULINUX_LINUX_HARDWARE` gates all Linux-specific code paths (LinuxGPIOPin, LinuxHardwareSPI, LinuxHardwareI2C, LinuxSerial). Simulated variants compile unconditionally.

### VFS

The VFS maps the Arduino `FS` API to POSIX calls on a host directory. Access it via `ArduLinuxFS` (the `fs::FS` object) or directly via `ardulinuxVFS`. The mount point is set by `main()` before `ardulinuxSetup()` runs, so it is available during setup.

### Submodules

- `ArduinoCore-API/` — upstream Arduino API headers
- `libraries/WiFi` — arduino-libraries/WiFi (pinned at `c75a5e8`)
