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
    ${CMAKE_SOURCE_DIR}/cores/ardulinux/simulated/SimHardwareI2C.cpp
)

target_link_libraries(ardulinux-tests PRIVATE ardulinux-base Catch2::Catch2WithMain)

# Coverage: instrument test sources and link the gcov runtime.
# PRIVATE so Catch2::Catch2WithMain is not affected.
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    target_compile_options(ardulinux-tests PRIVATE --coverage -O0 -g)
    target_link_options(ardulinux-tests PRIVATE --coverage)
endif()

include(Catch)
catch_discover_tests(ardulinux-tests)
