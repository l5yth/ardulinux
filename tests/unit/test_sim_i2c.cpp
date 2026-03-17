// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <catch2/catch_test_macros.hpp>
#include "simulated/SimHardwareI2C.h"

// ─── SimHardwareI2C ───────────────────────────────────────────────────────────
//
// SimHardwareI2C is the no-op I2C stub used in simulation (no libgpiod/i2c-dev).
// Tests verify that all methods return the documented "not supported" values
// without crashing, satisfying the ARDULINUX_HARDWARE=off build contract.

TEST_CASE("SimHardwareI2C::endTransmission returns I2cAddrNAK", "[i2c][sim]") {
    arduino::SimHardwareI2C i2c;
    i2c.beginTransmission(0x42);  // no-op; sets target address
    CHECK(i2c.endTransmission() == arduino::I2cAddrNAK);
}

TEST_CASE("SimHardwareI2C::endTransmission(stopBit) returns I2cAddrNAK", "[i2c][sim]") {
    arduino::SimHardwareI2C i2c;
    i2c.beginTransmission(0x42);
    CHECK(i2c.endTransmission(true) == arduino::I2cAddrNAK);
    CHECK(i2c.endTransmission(false) == arduino::I2cAddrNAK);
}

TEST_CASE("SimHardwareI2C::requestFrom returns 0 (no bytes received)", "[i2c][sim]") {
    arduino::SimHardwareI2C i2c;
    CHECK(i2c.requestFrom((uint8_t)0x42, (size_t)10, true) == 0);
    CHECK(i2c.requestFrom((uint8_t)0x42, (size_t)10) == 0);
}

TEST_CASE("SimHardwareI2C::write returns 0 (no hardware)", "[i2c][sim]") {
    arduino::SimHardwareI2C i2c;
    CHECK(i2c.write((uint8_t)0x55) == 0);
}

TEST_CASE("SimHardwareI2C::write(buf, size) returns 0 (no hardware)", "[i2c][sim]") {
    arduino::SimHardwareI2C i2c;
    const uint8_t buf[] = {0x01, 0x02, 0x03};
    CHECK(i2c.write(buf, sizeof(buf)) == 0);
}

TEST_CASE("SimHardwareI2C::read returns -1 (no hardware)", "[i2c][sim]") {
    arduino::SimHardwareI2C i2c;
    CHECK(i2c.read() == -1);
}

TEST_CASE("SimHardwareI2C::peek returns -1 (no hardware)", "[i2c][sim]") {
    arduino::SimHardwareI2C i2c;
    CHECK(i2c.peek() == -1);
}

TEST_CASE("SimHardwareI2C::available returns 0 (no hardware)", "[i2c][sim]") {
    arduino::SimHardwareI2C i2c;
    CHECK(i2c.available() == 0);
}

TEST_CASE("SimHardwareI2C::beginTransmission is a no-op", "[i2c][sim]") {
    arduino::SimHardwareI2C i2c;
    // Just verify it does not crash for any address.
    i2c.beginTransmission(0x00);
    i2c.beginTransmission(0x7F);
}

// ─── ResultI2c enum ───────────────────────────────────────────────────────────

TEST_CASE("ResultI2c enum values match Arduino Wire API spec", "[i2c]") {
    // The Wire API documents these exact integer values; they must not drift.
    CHECK(arduino::I2cSuccess   == 0);
    CHECK(arduino::I2cTooLong   == 1);
    CHECK(arduino::I2cAddrNAK   == 2);
    CHECK(arduino::I2cDataNAK   == 3);
    CHECK(arduino::I2cOtherError == 4);
}
