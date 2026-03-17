// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <catch2/catch_test_macros.hpp>
#include "linux/LinuxSerial.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

// ─── Pre-begin state (serial_port == -1) ─────────────────────────────────────
//
// These tests exercise the fixed return-value handling in read() and write()
// when the port has never been opened.  Before the fix, the ::read / ::write
// return values were ignored (triggering -Wunused-result); the functions now
// check the return value and propagate the error correctly.

TEST_CASE("LinuxSerial::read returns -1 when port not open", "[serial]") {
    arduino::LinuxSerial serial;
    // serial_port == -1; ::read(-1, ...) fails with EBADF → n != 1 → return -1
    CHECK(serial.read() == -1);
}

TEST_CASE("LinuxSerial::write returns 0 when port not open", "[serial]") {
    arduino::LinuxSerial serial;
    // serial_port == -1; ::write(-1, ...) fails with EBADF → n != 1 → return 0
    CHECK(serial.write(0x42) == 0);
}

TEST_CASE("LinuxSerial::available returns 0 when port not open", "[serial]") {
    arduino::LinuxSerial serial;
    CHECK(serial.available() == 0);
}

TEST_CASE("LinuxSerial::operator bool is false before begin()", "[serial]") {
    arduino::LinuxSerial serial;
    CHECK(!serial);
}

// ─── PTY-backed I/O ──────────────────────────────────────────────────────────
//
// Uses posix_openpt() (standard POSIX, no -lutil required) to open a
// pseudoterminal pair.  The slave side behaves identically to a real serial
// device: it accepts termios configuration and supports non-blocking reads.

/**
 * Open a PTY master and return its fd.
 *
 * Writes the slave device path into @p slave_path.  Uses the standard
 * posix_openpt() / grantpt() / unlockpt() / ptsname() sequence so that no
 * extra libraries (-lutil) are required.
 *
 * @return Master fd on success, -1 on failure.
 */
static int open_pty_master(char *slave_path, size_t path_len) {
    int master = posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0) return -1;
    if (grantpt(master) != 0 || unlockpt(master) != 0) {
        close(master);
        return -1;
    }
    const char *name = ptsname(master);
    if (!name) {
        close(master);
        return -1;
    }
    strncpy(slave_path, name, path_len - 1);
    slave_path[path_len - 1] = '\0';
    return master;
}

TEST_CASE("LinuxSerial::operator bool is true after begin()", "[serial][pty]") {
    char slave_path[256];
    int master = open_pty_master(slave_path, sizeof(slave_path));
    REQUIRE(master >= 0);

    arduino::LinuxSerial serial;
    serial.setPath(slave_path);
    serial.begin(115200);

    CHECK(bool(serial));

    serial.end();
    close(master);
}

TEST_CASE("LinuxSerial::read returns byte written to PTY master", "[serial][pty]") {
    char slave_path[256];
    int master = open_pty_master(slave_path, sizeof(slave_path));
    REQUIRE(master >= 0);

    arduino::LinuxSerial serial;
    serial.setPath(slave_path);
    serial.begin(115200);
    REQUIRE(bool(serial));

    // Write a byte from the master side; it appears immediately in the slave's
    // input buffer (PTY delivery is synchronous within the kernel).
    uint8_t byte_out = 0x42;
    REQUIRE(::write(master, &byte_out, 1) == 1);

    // begin() sets VMIN=0/VTIME=0 (non-blocking) and disables echo, so the
    // byte is available immediately and is not echoed back to master.
    CHECK(serial.read() == 0x42);

    serial.end();
    close(master);
}

TEST_CASE("LinuxSerial::write sends byte visible from PTY master", "[serial][pty]") {
    char slave_path[256];
    int master = open_pty_master(slave_path, sizeof(slave_path));
    REQUIRE(master >= 0);

    arduino::LinuxSerial serial;
    serial.setPath(slave_path);
    serial.begin(115200);
    REQUIRE(bool(serial));

    // Write from the slave side; data appears in the master's input queue.
    CHECK(serial.write(0xAB) == 1);

    uint8_t buf = 0;
    REQUIRE(::read(master, &buf, 1) == 1);
    CHECK(buf == 0xAB);

    serial.end();
    close(master);
}

TEST_CASE("LinuxSerial::read returns -1 when no data available on PTY", "[serial][pty]") {
    char slave_path[256];
    int master = open_pty_master(slave_path, sizeof(slave_path));
    REQUIRE(master >= 0);

    arduino::LinuxSerial serial;
    serial.setPath(slave_path);
    serial.begin(115200);
    REQUIRE(bool(serial));

    // No data written; VMIN=0/VTIME=0 makes read() return immediately with n=0
    // which is != 1, so read() returns -1.
    CHECK(serial.read() == -1);

    serial.end();
    close(master);
}
