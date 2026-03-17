// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for LinuxHardwareI2C RX buffer logic.
//
// These tests exercise the read()/available() buffer path in isolation from
// real I2C hardware.  The module-level RX buffer variables (RXbuf, RXlen,
// RXindex) are pre-populated via extern declarations so that read() and
// available() can be driven through the buffer branch without ever calling
// begin() or touching /dev/i2c-N.
//
// Key coverage: the (int)RXindex cast that prevents a latent size_t underflow
// when checking whether the buffer has remaining data.

#include <catch2/catch_test_macros.hpp>
#include "linux/LinuxHardwareI2C.h"

// Expose the module-level buffer variables defined in LinuxHardwareI2C.cpp.
// These are implementation details intentionally not exported from the header;
// declaring them extern here is the standard C++ technique for white-box
// unit testing without modifying production code.
namespace arduino {
    extern char  RXbuf[1000];
    extern int   RXlen;
    extern size_t RXindex;
}

// ─── RX buffer drain ─────────────────────────────────────────────────────────

TEST_CASE("LinuxHardwareI2C::read drains RX buffer in order", "[i2c][linux]") {
    arduino::RXbuf[0] = 0x11;
    arduino::RXbuf[1] = 0x22;
    arduino::RXbuf[2] = 0x33;
    arduino::RXlen   = 3;
    arduino::RXindex = 0;

    CHECK(arduino::Wire.read() == 0x11);
    CHECK(arduino::Wire.read() == 0x22);
    CHECK(arduino::Wire.read() == 0x33);

    // Buffer is reset after the last byte is consumed.
    CHECK(arduino::RXlen   == 0);
    CHECK(arduino::RXindex == 0);
}

TEST_CASE("LinuxHardwareI2C::available returns remaining byte count from RX buffer", "[i2c][linux]") {
    arduino::RXbuf[0] = 0xAA;
    arduino::RXbuf[1] = 0xBB;
    arduino::RXlen   = 2;
    arduino::RXindex = 0;

    CHECK(arduino::Wire.available() == 2);
    arduino::Wire.read();
    CHECK(arduino::Wire.available() == 1);
    arduino::Wire.read();
    // Buffer exhausted; available() falls through to ioctl(FIONREAD) which
    // returns 0 for fd 0 (stdin, no data pending).
    CHECK(arduino::Wire.available() == 0);
}

// ─── Signed/unsigned underflow guard ─────────────────────────────────────────
//
// Before the fix, available() and read() computed (RXlen - RXindex) with
// unsigned size_t arithmetic.  When RXindex > RXlen (an abnormal state that
// can arise from double-read or index corruption), the subtraction wraps to a
// huge positive value, incorrectly signalling that buffer data is available.
//
// The fix casts RXindex to int so the subtraction is signed: a negative result
// correctly evaluates to != 0 as false (zero bytes available).

TEST_CASE("LinuxHardwareI2C::available returns 0 when RXindex > RXlen (underflow guard)", "[i2c][linux]") {
    arduino::RXlen   = 0;
    arduino::RXindex = 1;  // Abnormal: index past end

    // Without the (int) cast: 0 - 1 as size_t = UINT_MAX → wrongly != 0.
    // With the cast:           0 - (int)1 = -1 → == 0 is false → branch not taken → returns 0.
    CHECK(arduino::Wire.available() == 0);

    // Reset for subsequent tests.
    arduino::RXindex = 0;
}

TEST_CASE("LinuxHardwareI2C::read skips buffer when RXindex > RXlen (underflow guard)", "[i2c][linux]") {
    // Fill the buffer with a sentinel to detect any spurious buffer read.
    // Before the fix (size_t arithmetic, != 0 guard): RXlen - RXindex would
    // produce SIZE_MAX, the guard would be true, and RXbuf[1] (0xFF) would be
    // returned as if valid data were present.
    // After the fix (signed arithmetic, > 0 guard): the result is -1, which
    // is not > 0, so the buffer branch is correctly skipped.
    arduino::RXbuf[1] = 0xFF;  // sentinel at the index that would be wrongly read
    arduino::RXlen   = 0;
    arduino::RXindex = 1;  // Abnormal state: index past end

    int result = arduino::Wire.read();

    // The sentinel value must NOT be returned; the buffer path must be skipped.
    CHECK(result != 0xFF);

    // Reset for subsequent tests.
    arduino::RXlen   = 0;
    arduino::RXindex = 0;
}
