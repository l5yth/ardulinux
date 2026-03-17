// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for millis() and micros() in cores/ardulinux/linux/millis.cpp.
//
// These functions use gettimeofday() with lazy first-call initialisation.
// The only observable property we can test portably is monotonicity: two
// successive calls must return a non-decreasing value.

#include <catch2/catch_test_macros.hpp>

extern "C" unsigned long millis(void);
extern "C" unsigned long micros(void);

TEST_CASE("millis returns a non-decreasing value on successive calls", "[time]") {
    unsigned long t0 = millis();
    unsigned long t1 = millis();
    CHECK(t1 >= t0);
}

TEST_CASE("micros returns a non-decreasing value on successive calls", "[time]") {
    unsigned long t0 = micros();
    unsigned long t1 = micros();
    CHECK(t1 >= t0);
}

TEST_CASE("millis and micros are consistent (micros >= millis * 1000)", "[time]") {
    // Both functions sample time independently, but micros() must always be
    // at least as large as millis() * 1000 at the time it is called.
    unsigned long ms = millis();
    unsigned long us = micros();
    CHECK(us >= ms * 1000UL);
}
