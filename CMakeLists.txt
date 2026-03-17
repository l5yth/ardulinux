cmake_minimum_required(VERSION 3.17)
project(ArduLinux)

set(CMAKE_CXX_STANDARD 14)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGPIOD libgpiod)


# ─── ardulinux-base static library ───────────────────────────────────────────
# Core sources shared by the ArduLinux executable and the unit-test binary.
# Hardware-specific I2C/GPIO sources are added separately per target.
add_library(ardulinux-base STATIC
        # ArduinoCore-API
        cores/arduino/api/Common.cpp
        cores/arduino/api/IPAddress.cpp
        cores/arduino/api/Print.cpp
        cores/arduino/api/Stream.cpp
        cores/arduino/api/String.cpp

        # ArduLinux core
        cores/ardulinux/HardwareSPIStubs.cpp
        cores/ardulinux/itoa.cpp
        cores/ardulinux/dtostrf.c
        cores/ardulinux/logging.cpp
        cores/ardulinux/Utility.cpp
        cores/ardulinux/AppInfo.cpp
        cores/ardulinux/XDGDirs.cpp
        cores/ardulinux/ArduLinuxGPIO.cpp

        # Virtual filesystem
        cores/ardulinux/FS/ArduLinuxFS.cpp
        cores/ardulinux/FS/FS.cpp
        cores/ardulinux/FS/vfs_api.cpp

        # Linux platform (always compiled; provides delay, SPI, Serial, etc.)
        cores/ardulinux/linux/millis.cpp
        cores/ardulinux/linux/LinuxCommon.cpp
        cores/ardulinux/linux/LinuxSerial.cpp
        cores/ardulinux/linux/LinuxHardwareSPI.cpp
        cores/ardulinux/simulated/SimCommon.cpp
        cores/ardulinux/simulated/SimHardwareSPI.cpp
)

target_include_directories(ardulinux-base PUBLIC
        cores/arduino/api
        cores/ardulinux
        cores/ardulinux/FS
)

# ArduinoCore-API's Common.h re-declares atexit() without noexcept, which conflicts
# with glibc's noexcept declaration on GCC 15+. Defining HOST suppresses that
# declaration (guarded by #ifndef HOST) since we are a host machine build.
# Applied to ardulinux-base PUBLIC so all targets that link it inherit the definition.
target_compile_definitions(ardulinux-base PUBLIC HOST)

# Coverage build type:
#   PRIVATE compile — only ardulinux-base's own sources are instrumented; the
#     flag does NOT propagate to Catch2 (a FetchContent subproject that would
#     inherit PUBLIC flags).
#   INTERFACE link — propagates --coverage to every executable that links
#     ardulinux-base (ArduLinux, ardulinux-tests) so the gcov runtime
#     (libgcov) is pulled in and the __gcov_* stubs in the instrumented
#     objects are resolved. Catch2 is unaffected because it doesn't link
#     ardulinux-base.
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    target_compile_options(ardulinux-base PRIVATE --coverage -O0 -g)
    target_link_options(ardulinux-base INTERFACE --coverage)
endif()

if(LIBGPIOD_FOUND)
    message(STATUS "libgpiod found (${LIBGPIOD_VERSION}) -- enabling Linux hardware GPIO and I2C")
    target_compile_definitions(ardulinux-base PUBLIC ARDULINUX_HARDWARE)
    target_include_directories(ardulinux-base PUBLIC ${LIBGPIOD_INCLUDE_DIRS})
    target_link_libraries(ardulinux-base PUBLIC ${LIBGPIOD_LIBRARIES} i2c)
else()
    message(STATUS "libgpiod not found -- building with simulated GPIO and I2C only")
endif()

# ─── ArduLinux executable ────────────────────────────────────────────────────
add_executable(ArduLinux
        cores/ardulinux/main.cpp
        cores/ardulinux/main_entry.cpp
        example/src/test.cpp
)

target_link_libraries(ArduLinux PRIVATE ardulinux-base)

if(LIBGPIOD_FOUND)
    target_sources(ArduLinux PRIVATE
            cores/ardulinux/linux/gpio/LinuxGPIOPin.cpp
            cores/ardulinux/linux/LinuxHardwareI2C.cpp)
else()
    target_sources(ArduLinux PRIVATE
            cores/ardulinux/simulated/SimHardwareI2C.cpp)
endif()

# ─── Unit tests ──────────────────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests/unit)
