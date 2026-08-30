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

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <string>

namespace {

// A unique per-process console socket path so tests don't collide.
std::string unique_sock_path(const char *tag) {
    return std::string("/tmp/ardulinux-test-") + tag + "-" +
           std::to_string(getpid()) + ".sock";
}

// Connect a fresh AF_UNIX client to the SimSerial console at `path`.
int connect_console(const std::string &path) {
    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    REQUIRE(fd >= 0);
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
    REQUIRE(::connect(fd, (sockaddr *)&addr, sizeof(addr)) == 0);
    return fd;
}

// Poll available() until it reports at least `want` bytes or we give up.
int wait_available(arduino::SimSerial &sim, int want) {
    int avail = 0;
    for (int i = 0; i < 200 && avail < want; i++) {
        avail = sim.available();
        if (avail < want) usleep(1000);
    }
    return avail;
}

// Read exactly like the repeater's loop(): only call read() while available()
// reports bytes.  Polls until `terminator` is seen or we give up.  This is the
// shape the real consumer uses — never a bare read() drain.
std::string read_like_consumer(arduino::SimSerial &sim, char terminator) {
    std::string out;
    for (int i = 0; i < 400 && out.find(terminator) == std::string::npos; i++) {
        while (sim.available() > 0) {
            int c = sim.read();
            if (c < 0) break;  // contract: never happens while available()>0
            out += (char)c;
        }
        if (out.find(terminator) == std::string::npos) usleep(1000);
    }
    return out;
}

}  // namespace

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

// ─── Socket console ───────────────────────────────────────────────────────────
// When ARDULINUX_CONSOLE_SOCKET names a path, begin() opens a listening AF_UNIX
// socket there; a connected client can send commands to read() and receive
// bytes written via write().

TEST_CASE("SimSerial delivers bytes from a connected client to read()",
          "[serial][sim][socket]") {
    std::string path = unique_sock_path("read");
    setenv("ARDULINUX_CONSOLE_SOCKET", path.c_str(), 1);

    arduino::SimSerial sim;
    sim.begin(115200);

    int client = connect_console(path);
    REQUIRE(::write(client, "hi", 2) == 2);

    CHECK(wait_available(sim, 2) >= 2);
    CHECK(sim.read() == 'h');
    CHECK(sim.read() == 'i');

    ::close(client);
    sim.end();
    unsetenv("ARDULINUX_CONSOLE_SOCKET");
}

TEST_CASE("SimSerial maps newline to carriage return on input",
          "[serial][sim][socket]") {
    // The console CLI terminates a command on '\r'; standard tools send '\n'.
    // The console maps '\n' -> '\r' (1:1) so `echo cmd | socat` works.
    std::string path = unique_sock_path("nl");
    setenv("ARDULINUX_CONSOLE_SOCKET", path.c_str(), 1);

    arduino::SimSerial sim;
    sim.begin(115200);
    int client = connect_console(path);
    REQUIRE(::write(client, "hi\n", 3) == 3);
    REQUIRE(wait_available(sim, 3) >= 3);

    CHECK(sim.read() == 'h');
    CHECK(sim.read() == 'i');
    CHECK(sim.read() == '\r');  // '\n' translated to '\r'

    ::close(client);
    sim.end();
    unsetenv("ARDULINUX_CONSOLE_SOCKET");
}

TEST_CASE("SimSerial::write mirrors bytes to a connected client",
          "[serial][sim][socket]") {
    std::string path = unique_sock_path("write");
    setenv("ARDULINUX_CONSOLE_SOCKET", path.c_str(), 1);

    arduino::SimSerial sim;
    sim.begin(115200);
    int client = connect_console(path);

    // A write() must be accepted by acceptClient() first; poke the accept path.
    sim.available();
    sim.write((uint8_t)'O');
    sim.write((uint8_t)'K');

    char buf[2] = {0, 0};
    ssize_t got = 0;
    for (int i = 0; i < 200 && got < 2; i++) {
        ssize_t n = ::read(client, buf + got, 2 - got);
        if (n > 0) got += n; else usleep(1000);
    }
    CHECK(got == 2);
    CHECK(buf[0] == 'O');
    CHECK(buf[1] == 'K');

    ::close(client);
    sim.end();
    unsetenv("ARDULINUX_CONSOLE_SOCKET");
}

TEST_CASE("SimSerial::peek returns a byte without consuming it",
          "[serial][sim][socket]") {
    std::string path = unique_sock_path("peek");
    setenv("ARDULINUX_CONSOLE_SOCKET", path.c_str(), 1);

    arduino::SimSerial sim;
    sim.begin(115200);
    int client = connect_console(path);
    REQUIRE(::write(client, "Z", 1) == 1);
    REQUIRE(wait_available(sim, 1) >= 1);

    CHECK(sim.peek() == 'Z');   // peek does not consume
    CHECK(sim.peek() == 'Z');   // still there
    CHECK(sim.read() == 'Z');   // now consumed
    CHECK(sim.read() == -1);    // nothing left

    ::close(client);
    sim.end();
    unsetenv("ARDULINUX_CONSOLE_SOCKET");
}

TEST_CASE("SimSerial has no input before a client connects",
          "[serial][sim][socket]") {
    std::string path = unique_sock_path("noclient");
    setenv("ARDULINUX_CONSOLE_SOCKET", path.c_str(), 1);

    arduino::SimSerial sim;
    sim.begin(115200);

    CHECK(sim.available() == 0);
    CHECK(sim.read() == -1);
    CHECK(sim.peek() == -1);

    sim.end();
    unsetenv("ARDULINUX_CONSOLE_SOCKET");
}

TEST_CASE("SimSerial::write after client disconnect does not crash (no SIGPIPE)",
          "[serial][sim][socket]") {
    // The daemon writes logs continuously; a write to a closed peer must not
    // raise SIGPIPE and kill the process.
    std::string path = unique_sock_path("sigpipe");
    setenv("ARDULINUX_CONSOLE_SOCKET", path.c_str(), 1);

    arduino::SimSerial sim;
    sim.begin(115200);
    int client = connect_console(path);
    sim.available();  // accept the client
    sim.write((uint8_t)'x');
    ::close(client);  // peer goes away

    // Give the close time to propagate, then keep logging.  Without
    // MSG_NOSIGNAL this raises SIGPIPE and aborts the test binary.
    usleep(20000);
    for (int i = 0; i < 200; i++) sim.write((uint8_t)'y');
    CHECK(true);  // reaching here means no SIGPIPE

    sim.end();
    unsetenv("ARDULINUX_CONSOLE_SOCKET");
}

TEST_CASE("SimSerial serves a new client after one disconnects (consumer loop shape)",
          "[serial][sim][socket]") {
    // Reap must happen through available() alone — the consumer never calls a
    // bare read() to drain EOF.
    std::string path = unique_sock_path("consumer");
    setenv("ARDULINUX_CONSOLE_SOCKET", path.c_str(), 1);

    arduino::SimSerial sim;
    sim.begin(115200);

    int c1 = connect_console(path);
    REQUIRE(::write(c1, "a\n", 2) == 2);
    CHECK(read_like_consumer(sim, '\r') == "a\r");
    ::close(c1);

    // Consumer keeps polling available() (never a direct read()); this must
    // notice the dead peer and drop it so the next client can be accepted.
    for (int i = 0; i < 50; i++) { sim.available(); usleep(1000); }

    int c2 = connect_console(path);
    REQUIRE(::write(c2, "b\n", 2) == 2);
    CHECK(read_like_consumer(sim, '\r') == "b\r");

    ::close(c2);
    sim.end();
    unsetenv("ARDULINUX_CONSOLE_SOCKET");
}

TEST_CASE("SimSerial::begin honors ARDULINUX_CONSOLE_SOCKET and restricts perms",
          "[serial][sim][socket]") {
    std::string path = unique_sock_path("perms");
    setenv("ARDULINUX_CONSOLE_SOCKET", path.c_str(), 1);

    arduino::SimSerial sim;
    sim.begin(115200);

    struct stat st{};
    REQUIRE(::stat(path.c_str(), &st) == 0);
    CHECK(S_ISSOCK(st.st_mode));
    CHECK((st.st_mode & 0777) == 0600);  // owner-only: the god-mode gate

    sim.end();
    CHECK(::stat(path.c_str(), &st) != 0);  // end() unlinks the socket

    unsetenv("ARDULINUX_CONSOLE_SOCKET");
}
