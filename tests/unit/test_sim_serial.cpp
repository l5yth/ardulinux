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
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

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

// ─── stdout buffering (SimSerial constructor) ─────────────────────────────────
//
// The SimSerial constructor line-buffers stdout once, at static init, so that
// Serial output reaches a non-TTY sink (pipe, file, journald) on each newline
// rather than sitting in stdio's full-buffer until exit.  Asserted
// behaviourally: point fd 1 at a pipe, emit a line, and read it straight back.
// The read must happen before any fflush() — flushing by hand would push the
// line through even when stdout is fully buffered, and mask a regression.

TEST_CASE("SimSerial constructor line-buffers stdout", "[serial][sim][buffering]") {
    // Drain anything the harness left pending so the pipe sees only our line.
    fflush(stdout);

    int pipefd[2];
    REQUIRE(pipe(pipefd) == 0);
    // A fully-buffered stdout puts nothing in the pipe; the read has to report
    // that rather than block forever waiting for a writer that never writes.
    REQUIRE(fcntl(pipefd[0], F_SETFL, O_NONBLOCK) == 0);

    int saved_stdout = dup(STDOUT_FILENO);
    REQUIRE(saved_stdout != -1);
    REQUIRE(dup2(pipefd[1], STDOUT_FILENO) != -1);

    // The trailing newline is what triggers the flush under _IOLBF.
    arduino::Serial.println("LINEBUF");

    char buf[64] = {0};
    ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);

    // Restore fd 1 before asserting, so a failure still reports to the console.
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    close(pipefd[1]);
    close(pipefd[0]);

    REQUIRE(n > 0);
    CHECK(std::string(buf).find("LINEBUF") != std::string::npos);
}
