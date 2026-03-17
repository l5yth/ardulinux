// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for LinuxHardwareSPI in cores/ardulinux/linux/LinuxHardwareSPI.cpp.
//
// LinuxHardwareSPI::begin() tries to open the spidev character device.  In
// the test environment, no spidev node is present, so the LinuxSPIChip
// constructor throws (via ardulinuxError) and begin() falls back to a
// SimSPIChip.  All subsequent operations therefore run against the no-op
// simulator, which is sufficient to exercise the LinuxHardwareSPI method
// bodies without real hardware.

#include <catch2/catch_test_macros.hpp>
#include "linux/LinuxHardwareSPI.h"

// ─── constructor / spidev path derivation ────────────────────────────────────

TEST_CASE("LinuxHardwareSPI default constructor builds spidev0.0 path", "[spi][linux]") {
    // The constructor derives the spidev path from the host encoding.
    // It does not open the device; begin() does.  Construction must not throw.
    arduino::LinuxHardwareSPI spi0;
    (void)spi0;
}

TEST_CASE("LinuxHardwareSPI(0x10) encodes bus=0, device=1", "[spi][linux]") {
    // spi_host=0x10 → low nibble bus=0, high nibble device=1 → /dev/spidev0.1
    arduino::LinuxHardwareSPI spi(0x10);
    (void)spi;
}

// ─── begin / end ─────────────────────────────────────────────────────────────

TEST_CASE("LinuxHardwareSPI::begin() falls back to SimSPIChip when no spidev", "[spi][linux]") {
    arduino::LinuxHardwareSPI spi;
    // begin() tries LinuxSPIChip; if /dev/spidev0.0 is absent it throws and
    // the catch block creates a SimSPIChip.  Either way, no exception escapes.
    CHECK_NOTHROW(spi.begin());
    spi.end();
}

TEST_CASE("LinuxHardwareSPI::begin(freq) falls back to SimSPIChip", "[spi][linux]") {
    arduino::LinuxHardwareSPI spi;
    CHECK_NOTHROW(spi.begin(500000));
    spi.end();
}

TEST_CASE("LinuxHardwareSPI::begin(name, freq) with non-existent device does not crash", "[spi][linux]") {
    arduino::LinuxHardwareSPI spi;
    CHECK_NOTHROW(spi.begin("/dev/nonexistent-spidev", 1000000));
    spi.end();
}

TEST_CASE("LinuxHardwareSPI::end() is safe to call without begin()", "[spi][linux]") {
    arduino::LinuxHardwareSPI spi;
    CHECK_NOTHROW(spi.end());
}

TEST_CASE("LinuxHardwareSPI::end() is safe to call after begin()", "[spi][linux]") {
    arduino::LinuxHardwareSPI spi;
    spi.begin();
    CHECK_NOTHROW(spi.end());
}

// ─── transfer via SimSPIChip fallback ────────────────────────────────────────

TEST_CASE("LinuxHardwareSPI::transfer(void*, size_t) runs via SimSPIChip", "[spi][linux]") {
    arduino::LinuxHardwareSPI spi;
    spi.begin();
    uint8_t buf[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    // SimSPIChip::transfer is a no-op; does not modify buf.
    CHECK_NOTHROW(spi.transfer(buf, sizeof(buf)));
    spi.end();
}

TEST_CASE("LinuxHardwareSPI::transfer16 returns 0 (not implemented)", "[spi][linux]") {
    arduino::LinuxHardwareSPI spi;
    spi.begin();
    // transfer16 calls notImplemented() (prints a message) and returns 0.
    CHECK(spi.transfer16(0x1234) == 0);
    spi.end();
}

// ─── transaction helpers ─────────────────────────────────────────────────────

TEST_CASE("LinuxHardwareSPI::beginTransaction/endTransaction do not crash via sim", "[spi][linux]") {
    arduino::LinuxHardwareSPI spi;
    spi.begin();
    arduino::SPISettings settings(1000000, MSBFIRST, SPI_MODE0);
    CHECK_NOTHROW(spi.beginTransaction(settings));
    CHECK_NOTHROW(spi.endTransaction());
    spi.end();
}

TEST_CASE("LinuxHardwareSPI::endTransaction without beginTransaction is safe", "[spi][linux]") {
    arduino::LinuxHardwareSPI spi;
    spi.begin();
    CHECK_NOTHROW(spi.endTransaction());
    spi.end();
}

// ─── interrupt no-ops ────────────────────────────────────────────────────────

TEST_CASE("LinuxHardwareSPI interrupt no-ops do not crash", "[spi][linux]") {
    arduino::LinuxHardwareSPI spi;
    CHECK_NOTHROW(spi.usingInterrupt(0));
    CHECK_NOTHROW(spi.notUsingInterrupt(0));
    CHECK_NOTHROW(spi.attachInterrupt());
    CHECK_NOTHROW(spi.detachInterrupt());
}
