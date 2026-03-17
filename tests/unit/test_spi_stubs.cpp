// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for the HardwareSPI base-class stubs in HardwareSPIStubs.cpp.
//
// ArduinoCore-API declares the HardwareSPI virtuals without definitions,
// expecting each platform to supply them.  HardwareSPIStubs.cpp provides
// do-nothing bodies so that the vtable can be built.
//
// LinuxHardwareSPI overrides all stubs at runtime, so they are never called
// through that path.  The only way to exercise the stub bodies is to call
// them through a subclass that does NOT override them — which is what
// BareSPI does below.

#include <catch2/catch_test_macros.hpp>
#include "HardwareSPI.h"

// BareSPI: minimal concrete subclass with no overrides.
// Every virtual method dispatches straight to the stub in HardwareSPIStubs.cpp.
class BareSPI : public arduino::HardwareSPI {};

TEST_CASE("HardwareSPI::transfer(uint8_t) stub returns 0", "[spi][stubs]") {
    BareSPI spi;
    CHECK(spi.transfer((uint8_t)0xAB) == 0);
}

TEST_CASE("HardwareSPI::transfer16 stub returns 0", "[spi][stubs]") {
    BareSPI spi;
    CHECK(spi.transfer16((uint16_t)0x1234) == 0);
}

TEST_CASE("HardwareSPI::transfer(void*, size_t) stub does not crash", "[spi][stubs]") {
    BareSPI spi;
    uint8_t buf[4] = {1, 2, 3, 4};
    CHECK_NOTHROW(spi.transfer(buf, sizeof(buf)));
}

TEST_CASE("HardwareSPI::usingInterrupt stub does not crash", "[spi][stubs]") {
    BareSPI spi;
    CHECK_NOTHROW(spi.usingInterrupt(0));
}

TEST_CASE("HardwareSPI::notUsingInterrupt stub does not crash", "[spi][stubs]") {
    BareSPI spi;
    CHECK_NOTHROW(spi.notUsingInterrupt(0));
}

TEST_CASE("HardwareSPI::beginTransaction stub does not crash", "[spi][stubs]") {
    BareSPI spi;
    CHECK_NOTHROW(spi.beginTransaction(arduino::SPISettings(1000000, MSBFIRST, SPI_MODE0)));
}

TEST_CASE("HardwareSPI::endTransaction stub does not crash", "[spi][stubs]") {
    BareSPI spi;
    CHECK_NOTHROW(spi.endTransaction());
}

TEST_CASE("HardwareSPI::attachInterrupt stub does not crash", "[spi][stubs]") {
    BareSPI spi;
    CHECK_NOTHROW(spi.attachInterrupt());
}

TEST_CASE("HardwareSPI::detachInterrupt stub does not crash", "[spi][stubs]") {
    BareSPI spi;
    CHECK_NOTHROW(spi.detachInterrupt());
}

TEST_CASE("HardwareSPI::begin stub does not crash", "[spi][stubs]") {
    BareSPI spi;
    CHECK_NOTHROW(spi.begin());
}

TEST_CASE("HardwareSPI::end stub does not crash", "[spi][stubs]") {
    BareSPI spi;
    CHECK_NOTHROW(spi.end());
}
