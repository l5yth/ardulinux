// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for the Linux platform helpers in LinuxCommon.cpp:
//   delay(), delayMicroseconds(), yield(), random(), randomSeed().
//
// All tests pass 0 (or trivial values) to the delay functions so that the
// test suite completes instantly while still exercising the function bodies.

#include <catch2/catch_test_macros.hpp>
#include "ArduLinuxGPIO.h"

// delay(), delayMicroseconds(), yield(), random(), randomSeed() are declared
// in Common.h (ArduinoCore-API) which is pulled in transitively by
// ArduLinuxGPIO.h; no extra forward declarations are needed here.

// ─── delay ───────────────────────────────────────────────────────────────────

TEST_CASE("delay(0) returns immediately without crashing", "[common]") {
    // Ensure realHardware is off so gpioIdle() is not called (it requires
    // pins to be initialised via gpioInit()).
    bool saved = realHardware;
    realHardware = false;
    CHECK_NOTHROW(delay(0));
    realHardware = saved;
}

TEST_CASE("delay(0) with realHardware=true calls gpioIdle() without crash", "[common]") {
    // gpioInit() populates the pin table; gpioIdle() must be safe to call.
    gpioInit(2);
    bool saved = realHardware;
    realHardware = true;
    CHECK_NOTHROW(delay(0));
    realHardware = saved;
}

// ─── delayMicroseconds ───────────────────────────────────────────────────────

TEST_CASE("delayMicroseconds(0) returns immediately without crashing", "[common]") {
    CHECK_NOTHROW(delayMicroseconds(0));
}

// ─── yield ───────────────────────────────────────────────────────────────────

TEST_CASE("yield does not crash", "[common]") {
    CHECK_NOTHROW(yield());
}

// ─── random ──────────────────────────────────────────────────────────────────

TEST_CASE("random(max) returns a value in [0, max)", "[common]") {
    long r = random(100);
    CHECK(r >= 0);
    CHECK(r < 100);
}

TEST_CASE("random(min, max) returns a value in [min, max)", "[common]") {
    long r = random(5, 15);
    CHECK(r >= 5);
    CHECK(r < 15);
}

TEST_CASE("random(min, max) returns min when range is empty (min == max)", "[common]") {
    CHECK(random(7, 7) == 7);
}

TEST_CASE("random(min, max) returns min when range is inverted (min > max)", "[common]") {
    CHECK(random(10, 3) == 10);
}

// ─── randomSeed ──────────────────────────────────────────────────────────────

TEST_CASE("randomSeed does not crash", "[common]") {
    CHECK_NOTHROW(randomSeed(42));
}

TEST_CASE("randomSeed(0) deterministically seeds the PRNG", "[common]") {
    // Two sequences seeded identically must produce the same first value.
    randomSeed(1234);
    long r1 = random(1000);
    randomSeed(1234);
    long r2 = random(1000);
    CHECK(r1 == r2);
}
