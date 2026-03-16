cmake_minimum_required(VERSION 3.17)
project(Meshduino)

set(CMAKE_CXX_STANDARD 14)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGPIOD libgpiod)

include_directories(cores/arduino/api)
include_directories(cores/meshduino)
include_directories(cores/meshduino/FS)

add_executable(Meshduino
        # ArduinoCore-API
        cores/arduino/api/Common.cpp
        cores/arduino/api/IPAddress.cpp
        cores/arduino/api/Print.cpp
        cores/arduino/api/Stream.cpp
        cores/arduino/api/String.cpp

        # Meshduino core
        cores/meshduino/itoa.cpp
        cores/meshduino/dtostrf.c
        cores/meshduino/logging.cpp
        cores/meshduino/Utility.cpp
        cores/meshduino/MeshduinoGPIO.cpp
        cores/meshduino/main.cpp

        # Virtual filesystem
        cores/meshduino/FS/MeshduinoFS.cpp
        cores/meshduino/FS/FS.cpp
        cores/meshduino/FS/vfs_api.cpp

        # Linux platform (always compiled; provide delay, SPI, Serial, etc.)
        cores/meshduino/linux/millis.cpp
        cores/meshduino/linux/LinuxCommon.cpp
        cores/meshduino/linux/LinuxSerial.cpp
        cores/meshduino/linux/LinuxHardwareSPI.cpp
        cores/meshduino/simulated/SimCommon.cpp
        cores/meshduino/simulated/SimHardwareSPI.cpp

        # Example application
        example/src/test.cpp
)

if(LIBGPIOD_FOUND)
    message(STATUS "libgpiod found (${LIBGPIOD_VERSION}) -- enabling Linux hardware GPIO and I2C")
    target_sources(Meshduino PRIVATE
            cores/meshduino/linux/gpio/LinuxGPIOPin.cpp
            cores/meshduino/linux/LinuxHardwareI2C.cpp)
    target_compile_definitions(Meshduino PRIVATE MESHDUINO_LINUX_HARDWARE)
    target_include_directories(Meshduino PRIVATE ${LIBGPIOD_INCLUDE_DIRS})
    target_link_libraries(Meshduino PRIVATE ${LIBGPIOD_LIBRARIES} i2c)
else()
    message(STATUS "libgpiod not found -- building with simulated GPIO and I2C only")
    target_sources(Meshduino PRIVATE
            cores/meshduino/simulated/SimHardwareI2C.cpp)
endif()
