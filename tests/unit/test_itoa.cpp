// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for itoa.cpp (itoa/ltoa/utoa/ultoa and radixToFmtString)
// and dtostrf.c (dtostrf).

#include <catch2/catch_test_macros.hpp>
#include <itoa.h>
#include <stdexcept>

extern "C" char *dtostrf(double val, signed char width, unsigned char prec, char *sout);

// ─── itoa ────────────────────────────────────────────────────────────────────

TEST_CASE("itoa converts decimal integer to string", "[itoa]") {
    char buf[32];
    CHECK(std::string(itoa(42, buf, 10)) == "42");
    CHECK(std::string(itoa(-7, buf, 10)) == "-7");
    CHECK(std::string(itoa(0, buf, 10)) == "0");
}

TEST_CASE("itoa converts integer to hexadecimal string", "[itoa]") {
    char buf[32];
    CHECK(std::string(itoa(255, buf, 16)) == "FF");
    CHECK(std::string(itoa(0, buf, 16)) == "0");
}

TEST_CASE("itoa converts integer to octal string", "[itoa]") {
    char buf[32];
    CHECK(std::string(itoa(8, buf, 8)) == "10");
}

TEST_CASE("itoa throws on unsupported radix", "[itoa]") {
    char buf[32];
    CHECK_THROWS_AS(itoa(1, buf, 2), std::runtime_error);
}

// ─── ltoa ────────────────────────────────────────────────────────────────────

TEST_CASE("ltoa converts long to decimal string", "[itoa]") {
    char buf[32];
    CHECK(std::string(ltoa(100000L, buf, 10)) == "100000");
    CHECK(std::string(ltoa(-1L, buf, 10)) == "-1");
}

TEST_CASE("ltoa converts long to hex string", "[itoa]") {
    char buf[32];
    CHECK(std::string(ltoa(0xABL, buf, 16)) == "AB");
}

// ─── utoa ────────────────────────────────────────────────────────────────────

TEST_CASE("utoa converts unsigned int to decimal string", "[itoa]") {
    char buf[32];
    CHECK(std::string(utoa(65535u, buf, 10)) == "65535");
}

TEST_CASE("utoa converts unsigned int to hex string", "[itoa]") {
    char buf[32];
    CHECK(std::string(utoa(0xFFu, buf, 16)) == "FF");
}

// ─── ultoa ───────────────────────────────────────────────────────────────────

TEST_CASE("ultoa converts unsigned long to decimal string", "[itoa]") {
    char buf[32];
    CHECK(std::string(ultoa(1000000UL, buf, 10)) == "1000000");
}

TEST_CASE("ultoa converts unsigned long to octal string", "[itoa]") {
    char buf[32];
    CHECK(std::string(ultoa(64UL, buf, 8)) == "100");
}

// ─── dtostrf ─────────────────────────────────────────────────────────────────

TEST_CASE("dtostrf converts double to fixed-point string", "[itoa]") {
    char buf[32];
    dtostrf(3.14159, 8, 3, buf);
    // Width=8, precision=3 → "   3.142" (space-padded to 8 chars)
    CHECK(std::string(buf).find("3.14") != std::string::npos);
}

TEST_CASE("dtostrf with zero produces '0.00'", "[itoa]") {
    char buf[32];
    dtostrf(0.0, 4, 2, buf);
    CHECK(std::string(buf).find("0.00") != std::string::npos);
}
