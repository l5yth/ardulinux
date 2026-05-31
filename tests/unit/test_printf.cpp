// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for Print::printf — the platform extension added in
// cores/ardulinux/Print.h.  printf() exists only in the ArduLinux shadow
// Print.h (upstream ArduinoCore-API has no Print::printf), so this test is its
// own executable whose include path puts cores/ardulinux first, compiling
// against the shadow in isolation (see tests/unit/CMakeLists.txt).

#include <catch2/catch_test_macros.hpp>

#include <string>

#include "Print.h"

namespace {
// Minimal concrete Print that records everything written through it.
class CapturePrint : public arduino::Print {
public:
    std::string data;
    size_t write(uint8_t b) override {
        data.push_back(static_cast<char>(b));
        return 1;
    }
    size_t write(const uint8_t *buf, size_t n) override {
        data.append(reinterpret_cast<const char *>(buf), n);
        return n;
    }
};
} // namespace

TEST_CASE("Print::printf formats and returns the number of bytes written", "[print][printf]") {
    CapturePrint p;
    const size_t n = p.printf("%s=%d", "x", 42);
    CHECK(p.data == "x=42");
    CHECK(n == 4);
}

TEST_CASE("Print::printf truncates to the buffer without over-reading", "[print][printf]") {
    CapturePrint p;
    const std::string big(500, 'A'); // far larger than the 256-byte buffer
    const size_t n = p.printf("%s", big.c_str());
    CHECK(p.data.size() == 255);            // clamped to sizeof(buf) - 1
    CHECK(n == 255);                        // returns the bytes actually written
    CHECK(p.data == std::string(255, 'A')); // exactly the bytes that fit, no garbage
}

TEST_CASE("Print::printf handles output exactly at the buffer boundary", "[print][printf]") {
    CapturePrint p;
    const std::string at255(255, 'B');
    const size_t n = p.printf("%s", at255.c_str());
    CHECK(p.data == at255);
    CHECK(n == 255);
}
