cmake_minimum_required(VERSION 3.17)
project(ArduLinux)

set(CMAKE_CXX_STANDARD 14)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGPIOD libgpiod)

include_directories(cores/arduino/api)
include_directories(cores/ardulinux)
include_directories(cores/ardulinux/FS)

add_executable(ArduLinux
        # ArduinoCore-API
        cores/arduino/api/Common.cpp
        cores/arduino/api/IPAddress.cpp
        cores/arduino/api/Print.cpp
        cores/arduino/api/Stream.cpp
        cores/arduino/api/String.cpp
)

# ArduinoCore-API's Common.h re-declares atexit() without noexcept, which conflicts
# with glibc's noexcept declaration on GCC 15+. Defining HOST suppresses that
# declaration (guarded by #ifndef HOST) since we are a host machine build.
# Applied globally because Common.h is included transitively by all ardulinux files.
target_compile_definitions(ArduLinux PRIVATE HOST)

target_sources(ArduLinux PRIVATE

        # ArduLinux core
        cores/ardulinux/HardwareSPIStubs.cpp
        cores/ardulinux/itoa.cpp
        cores/ardulinux/dtostrf.c
        cores/ardulinux/logging.cpp
        cores/ardulinux/Utility.cpp
        cores/ardulinux/ArduLinuxGPIO.cpp
        cores/ardulinux/main.cpp
        cores/ardulinux/main_entry.cpp

        # Virtual filesystem
        cores/ardulinux/FS/ArduLinuxFS.cpp
        cores/ardulinux/FS/FS.cpp
        cores/ardulinux/FS/vfs_api.cpp

        # Linux platform (always compiled; provide delay, SPI, Serial, etc.)
        cores/ardulinux/linux/millis.cpp
        cores/ardulinux/linux/LinuxCommon.cpp
        cores/ardulinux/linux/LinuxSerial.cpp
        cores/ardulinux/linux/LinuxHardwareSPI.cpp
        cores/ardulinux/simulated/SimCommon.cpp
        cores/ardulinux/simulated/SimHardwareSPI.cpp

        # Example application
        example/src/test.cpp
)

if(LIBGPIOD_FOUND)
    message(STATUS "libgpiod found (${LIBGPIOD_VERSION}) -- enabling Linux hardware GPIO and I2C")
    target_sources(ArduLinux PRIVATE
            cores/ardulinux/linux/gpio/LinuxGPIOPin.cpp
            cores/ardulinux/linux/LinuxHardwareI2C.cpp)
    target_compile_definitions(ArduLinux PRIVATE ARDULINUX_LINUX_HARDWARE)
    target_include_directories(ArduLinux PRIVATE ${LIBGPIOD_INCLUDE_DIRS})
    target_link_libraries(ArduLinux PRIVATE ${LIBGPIOD_LIBRARIES} i2c)
else()
    message(STATUS "libgpiod not found -- building with simulated GPIO and I2C only")
    target_sources(ArduLinux PRIVATE
            cores/ardulinux/simulated/SimHardwareI2C.cpp)
endif()
