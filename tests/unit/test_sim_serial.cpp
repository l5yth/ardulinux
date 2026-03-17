// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for SimSerial in cores/ardulinux/linux/LinuxSerial.cpp.
//
// SimSerial is the stdout-backed serial instance used as the Arduino Serial
// object on Linux.  All methods either no-op or delegate to putchar(); none
// of them require hardware or file descriptors.

#include <catch2/catch_test_macros.hpp>
#include "linux/LinuxSerial.h"

// ─── SimSerial API contract ───────────────────────────────────────────────────

TEST_CASE("SimSerial::operator bool is always true", "[serial][sim]") {
    arduino::SimSerial sim;
    CHECK(bool(sim) == true);
}

TEST_CASE("SimSerial::available always returns 0", "[serial][sim]") {
    arduino::SimSerial sim;
    CHECK(sim.available() == 0);
}

TEST_CASE("SimSerial::peek always returns -1", "[serial][sim]") {
    arduino::SimSerial sim;
    CHECK(sim.peek() == -1);
}

TEST_CASE("SimSerial::read always returns -1", "[serial][sim]") {
    arduino::SimSerial sim;
    CHECK(sim.read() == -1);
}

TEST_CASE("SimSerial::write returns 1 (byte routed to stdout)", "[serial][sim]") {
    arduino::SimSerial sim;
    // putchar() writes to stdout; the byte is visible in test output but the
    // return value is what we care about here.
    CHECK(sim.write((uint8_t)0x41) == 1);  // 'A'
}

TEST_CASE("SimSerial::begin(baud) is a no-op and does not crash", "[serial][sim]") {
    arduino::SimSerial sim;
    CHECK_NOTHROW(sim.begin(115200));
}

TEST_CASE("SimSerial::begin(baud, config) is a no-op and does not crash", "[serial][sim]") {
    arduino::SimSerial sim;
    CHECK_NOTHROW(sim.begin(9600, SERIAL_8N1));
}

TEST_CASE("SimSerial::end is a no-op and does not crash", "[serial][sim]") {
    arduino::SimSerial sim;
    CHECK_NOTHROW(sim.end());
}

TEST_CASE("SimSerial::flush is a no-op and does not crash", "[serial][sim]") {
    arduino::SimSerial sim;
    CHECK_NOTHROW(sim.flush());
}

// ─── Global Serial instance ───────────────────────────────────────────────────

TEST_CASE("arduino::Serial (global SimSerial) is always true", "[serial][sim]") {
    CHECK(bool(arduino::Serial) == true);
}

TEST_CASE("arduino::Serial::available returns 0", "[serial][sim]") {
    CHECK(arduino::Serial.available() == 0);
}
