include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

add_executable(ardulinux-tests
    test_gpio.cpp
    test_spi.cpp
    # Always use simulated I2C for tests (no real hardware needed)
    ../../cores/ardulinux/simulated/SimHardwareI2C.cpp
)

target_link_libraries(ardulinux-tests PRIVATE ardulinux-base Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(ardulinux-tests)
