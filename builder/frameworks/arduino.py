from os.path import join, isdir, exists, islink
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

# Expose ArduinoCore-API's headers to the compiler through a symlink overlay
# that omits Print.h, so `#include "Print.h"` falls through to our platform
# Print.h (cores/ardulinux/Print.h, which adds printf).  Unlike a one-shot
# copytree, the symlinks track the submodule's live content, so a submodule
# bump is always picked up with no stale copy.  The overlay is rebuilt only
# when the set of API headers changes, keeping incremental builds fast.
API_DIR = join(env.subst("$PROJECT_BUILD_DIR"), "patched_api")
wanted  = sorted(name for name in os.listdir(_API_DIR) if name != "Print.h")

def _overlay_current(d):
    if not isdir(d):
        return False
    entries = sorted(os.listdir(d))
    return entries == wanted and all(islink(join(d, n)) for n in entries)

if not _overlay_current(API_DIR):
    if exists(API_DIR):
        shutil.rmtree(API_DIR)
    os.makedirs(API_DIR)
    for name in wanted:
        os.symlink(join(_API_DIR, name), join(API_DIR, name))

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
# Not included: AsyncUDP.cpp (absent from CMakeLists.txt).
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
