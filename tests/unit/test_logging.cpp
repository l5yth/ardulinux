// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for logging.cpp:
//   arduino::log(), arduino::logv(), arduino::log_e(), arduino::log_w(),
//   arduino::log_i(), arduino::log_d(), arduino::log_v()
//
// All log functions route their output to Serial.write() (SimSerial).  The
// tests just verify that each function can be called without crashing; the
// output itself goes to stdout and is not asserted.

#include <catch2/catch_test_macros.hpp>
#include "logging.h"

// ─── arduino::log ─────────────────────────────────────────────────────────────

TEST_CASE("arduino::log does not crash with LogError level", "[logging]") {
    CHECK_NOTHROW(arduino::log(arduino::SysCore, arduino::LogError, "error %d", 1));
}

TEST_CASE("arduino::log does not crash with LogWarn level", "[logging]") {
    CHECK_NOTHROW(arduino::log(arduino::SysGPIO, arduino::LogWarn, "warn"));
}

TEST_CASE("arduino::log does not crash with LogInfo level", "[logging]") {
    CHECK_NOTHROW(arduino::log(arduino::SysI2C, arduino::LogInfo, "info %s", "test"));
}

TEST_CASE("arduino::log does not crash with LogDebug level", "[logging]") {
    CHECK_NOTHROW(arduino::log(arduino::SysSPI, arduino::LogDebug, "debug"));
}

TEST_CASE("arduino::log does not crash with LogVerbose level", "[logging]") {
    CHECK_NOTHROW(arduino::log(arduino::SysUnknown, arduino::LogVerbose, "verbose"));
}

// ─── arduino::logv ────────────────────────────────────────────────────────────

TEST_CASE("arduino::logv does not crash with a va_list", "[logging]") {
    // Call logv indirectly through log() which calls logv().  Directly calling
    // logv() with a va_list would require the caller to manage va_start/va_end
    // which is awkward to test; log() provides the end-to-end path.
    CHECK_NOTHROW(arduino::log(arduino::SysCurrent, arduino::LogInfo,
                               "value=%d str=%s float=%.2f", 42, "hello", 3.14));
}

TEST_CASE("arduino::logv truncates messages longer than 255 characters without crash", "[logging]") {
    // Build a 300-char format string.  logv() silently truncates to 255 chars.
    const std::string long_msg(300, 'x');
    CHECK_NOTHROW(arduino::log(arduino::SysApp0, arduino::LogInfo, "%s", long_msg.c_str()));
}

// ─── convenience wrappers ─────────────────────────────────────────────────────

TEST_CASE("arduino::log_e does not crash", "[logging]") {
    CHECK_NOTHROW(arduino::log_e("error message %d", 0));
}

TEST_CASE("arduino::log_w does not crash", "[logging]") {
    CHECK_NOTHROW(arduino::log_w("warn message"));
}

TEST_CASE("arduino::log_i does not crash", "[logging]") {
    CHECK_NOTHROW(arduino::log_i("info message %s", "ok"));
}

TEST_CASE("arduino::log_d does not crash", "[logging]") {
    CHECK_NOTHROW(arduino::log_d("debug message"));
}

TEST_CASE("arduino::log_v does not crash", "[logging]") {
    CHECK_NOTHROW(arduino::log_v("verbose message %u", 99u));
}
