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

#ifndef ARDULINUX_LINUXSERIAL_H
#define ARDULINUX_LINUXSERIAL_H

#include "HardwareSerial.h"
#include <string>

namespace arduino {

/**
 * HardwareSerial implementation backed by a POSIX serial (tty) device.
 *
 * Opens the device path set by setPath() (or the path passed to begin()),
 * configures raw mode with the requested baud rate via termios, and performs
 * non-blocking reads/writes using plain POSIX syscalls.
 *
 * Typical usage: call Serial1.setPath("/dev/ttyUSB0") in ardulinuxSetup(),
 * then let the application call Serial1.begin(115200) in setup().
 */
    class LinuxSerial : public HardwareSerial {
        int serial_port = -1;  ///< File descriptor; -1 when not yet opened
        std::string path;      ///< Device path set by setPath()
    public:
        /** Open with 8N1 framing at @p baudrate. */
        virtual void begin(unsigned long baudrate) { begin(baudrate, SERIAL_8N1); }

        /**
         * Open the serial device and configure termios.
         *
         * Sets raw mode (no echo, no canonical processing, no hardware flow
         * control) and applies the requested baud rate.  The @p config
         * argument is accepted for API compatibility but only SERIAL_8N1 is
         * currently applied.
         *
         * @param baudrate Baud rate (e.g. 115200).  Mapped to the closest
         *                 B* constant; falls back to the raw integer if none
         *                 matches.
         * @param config   Serial frame config (SERIAL_8N1 etc.) — see Arduino
         *                 HardwareSerial docs.
         */
        virtual void begin(unsigned long baudrate, uint16_t config);

        /**
         * Set the device path before calling begin().
         *
         * @param serialPath Path to the tty device (e.g. "/dev/ttyUSB0").
         *                   Empty string is ignored.
         * @return Always 0.
         */
        virtual int setPath(std::string serialPath);

        /** Close the serial device. */
        virtual void end();

        /**
         * Return the number of bytes available to read.
         *
         * Uses ioctl(FIONREAD) to query the kernel receive queue without
         * blocking.  Returns 0 if the file descriptor is invalid.
         */
        virtual int available(void);

        /** Not implemented; always returns -1. */
        virtual int peek(void);

        /**
         * Read one byte from the serial device.
         *
         * @return Byte value (0–255) on success, -1 if no data or on error.
         */
        virtual int read(void);

        /** Flush (no-op; tty is unbuffered at this level). */
        virtual void flush(void);

        /**
         * Write one byte to the serial device.
         *
         * @return 1 on success, 0 on write failure.
         */
        virtual size_t write(uint8_t);

        using Print::write; ///< Pull in write(str) and write(buf, size) from Print

        /** @return true if the serial port is open (file descriptor is valid). */
        virtual operator bool();
    };

/**
 * Simulated serial port that routes output to stdout.
 *
 * Used as the Serial (console) instance on Linux.  write() calls putchar()
 * so log output appears on the terminal.  read() always returns -1 (no
 * input from stdin is supported).
 */
    class SimSerial : public HardwareSerial {
    public:
        /** No-op; the simulated port is always "open". */
        virtual void begin(unsigned long baudrate) { begin(baudrate, SERIAL_8N1); }
        /** No-op. */
        virtual void begin(unsigned long baudrate, uint16_t config);
        /** No-op. */
        virtual void end();
        /** Always returns 0 (no input available). */
        virtual int available(void);
        /** Always returns -1. */
        virtual int peek(void);
        /** Always returns -1. */
        virtual int read(void);
        /** No-op. */
        virtual void flush(void);
        /**
         * Write one byte to stdout.
         *
         * @return Always 1.
         */
        virtual size_t write(uint8_t);
        using Print::write; ///< Pull in write(str) and write(buf, size) from Print
        /** Always returns true. */
        virtual operator bool();
    };

    extern LinuxSerial Serial1;  ///< Real serial port (configure via setPath())
    extern SimSerial Serial;     ///< Console (stdout) serial port
}

#endif //ARDULINUX_LINUXSERIAL_H
