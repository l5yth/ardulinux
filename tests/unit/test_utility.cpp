// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for Utility.cpp:
//   notImplemented(), ardulinuxError(), ardulinuxCheckNotNeg(),
//   ardulinuxCheckZero(), ardulinuxDebug().

#include <catch2/catch_test_macros.hpp>
#include "Utility.h"

#include <csignal>

// ─── notImplemented ───────────────────────────────────────────────────────────

TEST_CASE("notImplemented prints a message and does not throw", "[utility]") {
    // Just verify it doesn't crash; the message goes to stdout.
    CHECK_NOTHROW(notImplemented("test_feature"));
}

// ─── ardulinuxError ──────────────────────────────────────────────────────────

TEST_CASE("ardulinuxError throws an Exception with the formatted message", "[utility]") {
    bool caught = false;
    try {
        ardulinuxError("test error %d", 42);
    } catch (const Exception &e) {
        caught = true;
        CHECK(std::string(e.what()).find("test error 42") != std::string::npos);
    }
    CHECK(caught);
}

// ─── ardulinuxCheckNotNeg ─────────────────────────────────────────────────────

TEST_CASE("ardulinuxCheckNotNeg returns value when non-negative", "[utility]") {
    CHECK(ardulinuxCheckNotNeg(0, "zero is ok") == 0);
    CHECK(ardulinuxCheckNotNeg(5, "positive is ok") == 5);
}

TEST_CASE("ardulinuxCheckNotNeg throws Exception when negative", "[utility]") {
    CHECK_THROWS_AS(ardulinuxCheckNotNeg(-1, "negative fails"), Exception);
}

// ─── ardulinuxCheckZero ───────────────────────────────────────────────────────

TEST_CASE("ardulinuxCheckZero returns 0 when result is zero", "[utility]") {
    CHECK(ardulinuxCheckZero(0, "zero is ok") == 0);
}

TEST_CASE("ardulinuxCheckZero throws Exception when result is non-zero", "[utility]") {
    CHECK_THROWS_AS(ardulinuxCheckZero(1, "non-zero fails"), Exception);
    CHECK_THROWS_AS(ardulinuxCheckZero(-1, "negative fails too"), Exception);
}

// ─── ardulinuxDebug ───────────────────────────────────────────────────────────

TEST_CASE("ardulinuxDebug raises SIGINT (caught by test handler)", "[utility]") {
    // Install a handler that records the signal so the process is not killed.
    static volatile sig_atomic_t sigint_received;
    sigint_received = 0;

    auto prev = std::signal(SIGINT, [](int) { sigint_received = 1; });
    ardulinuxDebug();
    std::signal(SIGINT, prev);

    CHECK(sigint_received == 1);
}
