// ArduLinux - Arduino API for Linux
// Copyright (c) 2011-19 Arduino LLC.
// Copyright (c) 2020-23 Geeksville Industries, LLC
// Copyright (c) 2024-26 jp-bennett
// Copyright (c) 2026-27 l5yth
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#include "LinuxSerial.h"
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <string>
#include <sys/ioctl.h>

#include <stdio.h>

/** Module-level termios structure shared across begin() calls. */
struct termios tty;

namespace arduino {
    LinuxSerial Serial1;
    SimSerial Serial;
    // Reference: https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/

    void LinuxSerial::begin(unsigned long baudrate, uint16_t config) {
        serial_port = open(path.c_str(), O_RDWR);
        tcgetattr(serial_port, &tty);

        // Configure raw mode: no parity, 1 stop bit, 8 data bits, no flow control.
        // These settings match the most common Arduino Serial configuration.
        tty.c_cflag &= ~PARENB;         // No parity
        tty.c_cflag &= ~CSTOPB;         // 1 stop bit
        tty.c_cflag &= ~CSIZE;          // Clear data-size bits before setting CS8
        tty.c_cflag |= CS8;             // 8 data bits per byte
        tty.c_cflag &= ~CRTSCTS;        // No RTS/CTS hardware flow control
        tty.c_cflag |= CREAD | CLOCAL;  // Enable receiver; ignore modem control lines

        // Disable all line-discipline processing (raw mode).
        tty.c_lflag &= ~ICANON;  // Non-canonical (no line-by-line buffering)
        tty.c_lflag &= ~ECHO;    // No echo of received characters
        tty.c_lflag &= ~ECHOE;   // No erase character echo
        tty.c_lflag &= ~ECHONL;  // No newline echo
        tty.c_lflag &= ~ISIG;    // Do not generate signals for INTR/QUIT/SUSP

        // Disable software flow control.
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);

        // Disable all special input byte handling so bytes pass through unchanged.
        tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

        // Disable output post-processing so bytes are sent as-is.
        tty.c_oflag &= ~OPOST;   // No output processing
        tty.c_oflag &= ~ONLCR;   // No NL→CR/NL translation

        // Non-blocking reads: return immediately with whatever data is available.
        tty.c_cc[VTIME] = 0;  // No read timeout
        tty.c_cc[VMIN] = 0;   // Return even if zero bytes are available

        speed_t speed;
        switch(baudrate)
        {
#ifdef B1200
            case 1200:
                speed = B1200;
                break;
#endif
#ifdef B2400
            case 2400:
                speed = B2400;
                break;
#endif
#ifdef B4800
            case 4800:
                speed = B4800;
                break;
#endif
#ifdef B9600
            case 9600:
                speed = B9600;
                break;
#endif
#ifdef B19200
            case 19200:
                speed = B19200;
                break;
#endif
#ifdef B38400
            case 38400:
                speed = B38400;
                break;
#endif
#ifdef B57600
            case 57600:
                speed = B57600;
                break;
#endif
#ifdef B115200
            case 115200:
                speed = B115200;
                break;
#endif
#ifdef B230400
            case 230400:
                speed = B230400;
                break;
#endif
#ifdef B460800
            case 460800:
                speed = B460800;
                break;
#endif
#ifdef B500000
            case 500000:
                speed = B500000;
                break;
#endif
#ifdef B576000
            case 576000:
                speed = B576000;
                break;
#endif
#ifdef B921600
            case 921600:
                speed = B921600;
                break;
#endif
#ifdef B1000000
            case 1000000:
                speed = B1000000;
                break;
#endif
#ifdef B1152000
            case 1152000:
                speed = B1152000;
                break;
#endif
#ifdef B1500000
            case 1500000:
                speed = B1500000;
                break;
#endif
#ifdef B2000000
            case 2000000:
                speed = B2000000;
                break;
#endif
#ifdef B2500000
            case 2500000:
                speed = B2500000;
                break;
#endif
#ifdef B3000000
            case 3000000:
                speed = B3000000;
                break;
#endif
#ifdef B3500000
            case 3500000:
                speed = B3500000;
                break;
#endif
#ifdef B4000000
            case 4000000:
                speed = B4000000;
                break;
#endif
            default:
                speed = baudrate;
                break;
        }

        // Apply both input and output speeds, then commit all settings atomically.
        cfsetispeed(&tty, speed);
        cfsetospeed(&tty, speed);
        tcsetattr(serial_port, TCSANOW, &tty);  // TCSANOW: apply immediately
    }

    /**
     * Set the device path for the next begin() call.
     *
     * @param serialPath Path to the tty device.  Empty string is ignored.
     * @return Always 0.
     */
    int LinuxSerial::setPath(std::string serialPath) {
        if (serialPath != "")
            path = serialPath;
        return 0;
    }

    /** Close the serial port file descriptor. */
    void LinuxSerial::end() {
        if (serial_port != -1)
            close(serial_port);
    }

    /**
     * Return bytes available to read without blocking.
     *
     * FIONREAD queries the kernel tty receive queue size without consuming
     * data.  Returns 0 if the file descriptor is invalid (EBADF) rather than
     * propagating the error.
     */
    int LinuxSerial::available(void) {
        int bytes;
        int ret = ioctl(serial_port, FIONREAD, &bytes);
        if (ret == -1) {
            // ioctl failed: likely called before begin() opened the port.
            return 0;
        }
        return bytes;
    }

    /** Not implemented; peek requires buffering that this driver does not provide. */
    int LinuxSerial::peek(void) {
        return -1;
    }

    /**
     * Read one byte from the serial device.
     *
     * @return Byte value (0–255) on success, -1 if no data or on error.
     */
    int LinuxSerial::read(void) {
        uint8_t buf = 0;
        ssize_t n = ::read(serial_port, &buf, 1);
        return (n == 1) ? buf : -1;
    }

    /** No-op: data is written directly to the tty without userspace buffering. */
    void LinuxSerial::flush(void) {}

    /**
     * Write one byte to the serial device.
     *
     * @return 1 on success, 0 if the write syscall failed.
     */
    size_t LinuxSerial::write(uint8_t c) {
        ssize_t n = ::write(serial_port, &c, 1);
        return (n == 1) ? 1 : 0;
    }

    /** @return true if begin() has been called and the port is open. */
    LinuxSerial::operator bool() {
        return serial_port != -1;
    }



    // --- SimSerial: stdout-backed simulated serial (used as the console) ---

    /** No-op: simulated port requires no hardware setup. */
    void SimSerial::begin(unsigned long baudrate, uint16_t config) {}

    /** No-op: no file descriptor to close. */
    void SimSerial::end() {}

    /** Always returns 0: stdin is not monitored. */
    int SimSerial::available(void) {
        return 0;
    }

    /** Always returns -1: peek is not supported. */
    int SimSerial::peek(void) {
        return -1;
    }

    /** Always returns -1: reading from stdin is not supported. */
    int SimSerial::read(void) {
        return -1;
    }

    /** No-op: stdout is unbuffered at this level. */
    void SimSerial::flush(void) {}

    /**
     * Write one byte to stdout.
     *
     * Routes Arduino Serial.print/write output to the terminal so that log
     * messages (via logging.cpp) appear on the console.
     *
     * @return Always 1.
     */
    size_t SimSerial::write(uint8_t c) {
        putchar(c);
        return 1;
    }

    /** Always returns true: the simulated port is always ready. */
    SimSerial::operator bool() {
        return true;
    }
}
