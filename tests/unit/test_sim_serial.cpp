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

// ─── Global Serial instance ───────────────────────────────────────────────────

TEST_CASE("arduino::Serial (global SimSerial) is always true", "[serial][sim]") {
    CHECK(bool(arduino::Serial) == true);
}

TEST_CASE("arduino::Serial::available returns 0", "[serial][sim]") {
    CHECK(arduino::Serial.available() == 0);
}

// ─── stdout buffering and flush ───────────────────────────────────────────────
//
// SimSerial's constructor line-buffers stdout once, at static init, so Serial
// output reaches a non-TTY sink (pipe, file, journald) on each newline instead
// of sitting in stdio's full-buffer until exit.  flush() covers the remainder:
// a partial, newline-less line has nothing to trigger the line flush.
//
// Both are asserted behaviourally, by pointing fd 1 at a pipe and reading back
// what actually arrived.

namespace {

/**
 * Run @p emit with fd 1 pointed at a pipe; return the bytes that reached the
 * pipe by the time @p emit returned.
 *
 * The read is non-blocking and happens before any fflush() of our own: a
 * courtesy flush here would push the data through even when stdout is fully
 * buffered, and mask exactly the regression these tests exist to catch.
 *
 * @param emit  Callable that writes via arduino::Serial.
 * @return      What landed in the pipe; empty if stdout held onto it.
 */
template <typename F>
std::string capture_stdout_fd(F emit) {
    // Drain anything the harness left pending so the pipe sees only emit()'s bytes.
    fflush(stdout);

    int pipefd[2];
    REQUIRE(pipe(pipefd) == 0);
    // Nothing may ever be written; the read has to report that rather than block.
    REQUIRE(fcntl(pipefd[0], F_SETFL, O_NONBLOCK) == 0);

    int saved = dup(STDOUT_FILENO);
    REQUIRE(saved != -1);
    REQUIRE(dup2(pipefd[1], STDOUT_FILENO) != -1);

    emit();

    char buf[128] = {0};
    ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);

    // Restore fd 1 before the caller asserts, so failures still reach the console.
    dup2(saved, STDOUT_FILENO);
    close(saved);
    close(pipefd[1]);
    close(pipefd[0]);

    return (n > 0) ? std::string(buf, static_cast<size_t>(n)) : std::string();
}

}  // namespace

TEST_CASE("SimSerial constructor line-buffers stdout", "[serial][sim][buffering]") {
    // The trailing newline is what triggers the flush under _IOLBF.
    std::string out = capture_stdout_fd([] { arduino::Serial.println("LINEBUF"); });
    CHECK(out.find("LINEBUF") != std::string::npos);
}

TEST_CASE("SimSerial holds a partial line until flushed", "[serial][sim][buffering]") {
    // No newline, so line buffering has nothing to act on.  This is the control
    // for the flush test below: without it, that test would pass even if
    // stdout were unbuffered and flush() still a no-op.
    std::string out = capture_stdout_fd([] { arduino::Serial.print("HELD"); });
    CHECK(out.find("HELD") == std::string::npos);

    // Drop the held bytes on the real stdout rather than leaving them buffered.
    arduino::Serial.flush();
}

TEST_CASE("SimSerial::flush pushes a partial line out", "[serial][sim][buffering]") {
    std::string out = capture_stdout_fd([] {
        arduino::Serial.print("PARTIAL");
        arduino::Serial.flush();
    });
    CHECK(out.find("PARTIAL") != std::string::npos);
}
