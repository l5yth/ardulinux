// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for AppInfo.cpp (ardulinuxAppName, ardulinuxAppDescription).
//
// The test binary links ardulinux-base, which supplies the weak defaults.
// No application override is present in this translation unit, so the
// tests observe the platform default values.

#include <catch2/catch_test_macros.hpp>
#include "AppInfo.h"

#include <string>

// ─── ardulinuxAppName ─────────────────────────────────────────────────────────

TEST_CASE("ardulinuxAppName default is non-null", "[appinfo]") {
    CHECK(ardulinuxAppName != nullptr);
}

TEST_CASE("ardulinuxAppName default is 'ardulinux'", "[appinfo]") {
    CHECK(std::string(ardulinuxAppName) == "ardulinux");
}

TEST_CASE("ardulinuxAppName default is non-empty", "[appinfo]") {
    REQUIRE(ardulinuxAppName != nullptr);
    CHECK(ardulinuxAppName[0] != '\0');
}

// ─── ardulinuxAppDescription ──────────────────────────────────────────────────

TEST_CASE("ardulinuxAppDescription default is non-null", "[appinfo]") {
    CHECK(ardulinuxAppDescription != nullptr);
}

TEST_CASE("ardulinuxAppDescription default is non-empty", "[appinfo]") {
    REQUIRE(ardulinuxAppDescription != nullptr);
    CHECK(ardulinuxAppDescription[0] != '\0');
}
