from os.path import join, isdir, exists
import os
import shutil
import subprocess
from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

PLATFORM_DIR  = env.PioPlatform().get_dir()
_API_DIR      = join(PLATFORM_DIR, "cores", "arduino", "api")
ARDULINUX_DIR = join(PLATFORM_DIR, "cores", "ardulinux")

assert isdir(_API_DIR),      "ArduinoCore-API not found: " + _API_DIR
assert isdir(ARDULINUX_DIR), "ArduLinux core not found: "  + ARDULINUX_DIR

# Create a patched copy of ArduinoCore-API without Print.h so the include
# search falls through to our platform Print.h (which adds printf).
# This avoids forking ArduinoCore-API while keeping Print extensible.
API_DIR  = join(env.subst("$PROJECT_BUILD_DIR"), "patched_api")
SENTINEL = join(API_DIR, ".patched_from")
needs_rebuild = (
    not exists(API_DIR) or
    not exists(SENTINEL) or
    open(SENTINEL).read().strip() != _API_DIR
)
if needs_rebuild:
    if exists(API_DIR):
        shutil.rmtree(API_DIR)
    shutil.copytree(_API_DIR, API_DIR)
    os.remove(join(API_DIR, "Print.h"))
    with open(SENTINEL, "w") as f:
        f.write(_API_DIR)

# Detect libgpiod via pkg-config; fall back gracefully if pkg-config is absent.
try:
    has_libgpiod = subprocess.call(
        ["pkg-config", "--exists", "libgpiod"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    ) == 0
except FileNotFoundError:
    has_libgpiod = False

cppdefines = ["HOST", ("ARDUINO", 10810)]
cpppath    = [
    ARDULINUX_DIR,          # platform Print.h shadows ArduinoCore-API's
    join(ARDULINUX_DIR, "FS"),
    API_DIR,
    join(PLATFORM_DIR, "libraries", "Wire", "src"),
    join(PLATFORM_DIR, "libraries", "SPI", "src"),
]

if has_libgpiod:
    cppdefines.append("ARDULINUX_HARDWARE")
    raw = subprocess.check_output(["pkg-config", "--cflags", "libgpiod"]).decode().split()
    cpppath += [f[2:] for f in raw if f.startswith("-I")]

env.Append(CPPPATH=cpppath, CPPDEFINES=cppdefines)

if has_libgpiod:
    env.Append(LIBS=["gpiod", "i2c"])

# ─── ArduinoCore-API sources ──────────────────────────────────────────────────
env.BuildSources(
    join("$BUILD_DIR", "FrameworkArduinoAPI"),
    API_DIR,
    src_filter=[
        "-<*>",
        "+<Common.cpp>",
        "+<IPAddress.cpp>",
        "+<Print.cpp>",
        "+<Stream.cpp>",
        "+<String.cpp>",
    ],
)

# ─── ArduLinux core sources ───────────────────────────────────────────────────
# Source list mirrors CMakeLists.txt ardulinux-base — keep in sync.
# Not included: AsyncUDP.cpp, ArdulinuxPrint.cpp (absent from CMakeLists.txt).
ardulinux_filter = [
    "-<*>",
    "+<HardwareSPIStubs.cpp>",
    "+<itoa.cpp>",
    "+<dtostrf.c>",
    "+<logging.cpp>",
    "+<Utility.cpp>",
    "+<AppInfo.cpp>",
    "+<XDGDirs.cpp>",
    "+<ArduLinuxGPIO.cpp>",
    "+<main.cpp>",
    "+<main_entry.cpp>",
    # Virtual filesystem
    "+<FS/ArduLinuxFS.cpp>",
    "+<FS/FS.cpp>",
    "+<FS/vfs_api.cpp>",
    # Linux platform (always compiled: delay, SPI, Serial)
    "+<linux/millis.cpp>",
    "+<linux/LinuxCommon.cpp>",
    "+<linux/LinuxSerial.cpp>",
    "+<linux/LinuxHardwareSPI.cpp>",
    # Simulated fallbacks
    "+<simulated/SimCommon.cpp>",
    "+<simulated/SimHardwareSPI.cpp>",
]

if has_libgpiod:
    ardulinux_filter += [
        "+<linux/gpio/LinuxGPIOPin.cpp>",
        "+<linux/LinuxHardwareI2C.cpp>",
    ]
else:
    ardulinux_filter.append("+<simulated/SimHardwareI2C.cpp>")

env.BuildSources(
    join("$BUILD_DIR", "FrameworkArduLinux"),
    ARDULINUX_DIR,
    ardulinux_filter,
)
