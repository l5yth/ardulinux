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

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include "LinuxHardwareI2C.h"

extern "C"
{
    #include <linux/i2c-dev.h>
    #include <i2c/smbus.h>
}


namespace arduino {
    /** Transmit buffer: bytes queued by write() for the next endTransmission(). */
    char TXbuf[1000] = {0};
    /** Receive buffer: populated by requestFrom() and drained by read(). */
    char RXbuf[1000] = {0};
    /** Number of valid bytes currently in RXbuf (0 when empty). */
    int RXlen = 0;
    /** Read cursor into RXbuf; advances with each read() call. */
    size_t RXindex = 0;
    /** Number of bytes currently queued in TXbuf. */
    int requestedBytes = 0;
    /** Return value of the most recent I2C_SLAVE ioctl (informational). */
    int sessionstatus;
    /** Global Wire instance; opened on /dev/i2c-1 by default. */
    LinuxHardwareI2C Wire;
    /** Guards against double-open; set to true after the first begin() call. */
    bool hasBegun = false;

    /** Open the default I²C bus (/dev/i2c-1). */
    void LinuxHardwareI2C::begin() {
        begin("/dev/i2c-1");
    }

    /**
     * Open the given i2c-dev device.
     *
     * Only opens the file on the first call; subsequent calls are no-ops so
     * that libraries calling begin() multiple times don't leak descriptors.
     */
    void LinuxHardwareI2C::begin(const char * device) {
        if (!hasBegun) {
            i2c_file = open(device, O_RDWR);
            hasBegun = true;
        }
    }

    /** Close the i2c-dev file descriptor. */
    void LinuxHardwareI2C::end() {
        if (hasBegun) {
            close(i2c_file);
            hasBegun = false;
        }
    }

    /**
     * Select the target I²C device for the next transaction.
     *
     * I2C_SLAVE tells the kernel which 7-bit address to use for subsequent
     * read/write syscalls.  requestedBytes is reset so that the new
     * transaction starts with an empty TX buffer.
     */
    void LinuxHardwareI2C::beginTransmission(uint8_t address) {
        sessionstatus = ioctl(i2c_file, I2C_SLAVE, address);
        requestedBytes = 0;
    }

    /**
     * Flush the TX buffer to the device if stopBit is true.
     *
     * When stopBit is false the bytes remain in TXbuf for a combined
     * write-read via requestFrom() (restart without STOP).
     */
    uint8_t LinuxHardwareI2C::endTransmission(bool stopBit) {
        if (stopBit && requestedBytes) {
            int resp = ::write(i2c_file, TXbuf, requestedBytes);
            bool success = resp == requestedBytes;
            requestedBytes = 0;
            if (success)
                return 0;  // I2cSuccess
            else
                return 4;  // I2cOtherError
        }
        return 0;
    }

    /**
     * Issue an SMBus Quick Command (probes whether the address is present).
     *
     * @param toWrite I2C_SMBUS_WRITE (0) or I2C_SMBUS_READ (1).
     * @return 0 on ACK, negative errno on NACK or error.
     */
    int LinuxHardwareI2C::writeQuick(uint8_t toWrite) {
        int a = i2c_smbus_write_quick(i2c_file, toWrite);
        return a;
    }

    /** Queue one byte into TXbuf; returns 0 if the buffer is full. */
    size_t LinuxHardwareI2C::write(uint8_t toWrite) {
        if (requestedBytes >= (int)sizeof(TXbuf))
            return 0;
        TXbuf[requestedBytes] = toWrite;
        requestedBytes++;
        return 1;
    }

    /** Queue @p size bytes from @p buffer into TXbuf; returns bytes queued. */
    size_t LinuxHardwareI2C::write(const uint8_t *buffer, size_t size) {
        size_t written = 0;
        for (size_t i = 0; i < size; i++) {
            if (requestedBytes >= (int)sizeof(TXbuf))
                break;
            TXbuf[requestedBytes] = buffer[i];
            requestedBytes++;
            written++;
        }
        return written;
    }

    /**
     * Return one byte from the RX buffer, or read directly if the buffer is
     * empty.
     *
     * The RX buffer (RXbuf[0..RXlen-1]) is drained sequentially.  Once
     * exhausted (RXindex reaches RXlen), both indices are reset to 0 and
     * subsequent reads fall through to the raw ::read() path.
     */
    int LinuxHardwareI2C::read() {
        // (int)RXindex: cast to signed to prevent size_t wrap-around when
        // RXindex > RXlen — without the cast the subtraction would produce a
        // large positive value and the branch would be taken incorrectly.
        // This fixes a latent underflow bug in the original unsigned arithmetic.
        if (RXlen - (int)RXindex != 0) {
            int tmpVal = RXbuf[RXindex];
            RXindex++;
            // Reset when the last byte has been consumed.
            if (RXindex == (size_t)RXlen || RXindex > 999) {
                RXindex = 0;
                RXlen = 0;
            }
            return tmpVal;
        }
        // RX buffer exhausted; issue a raw 1-byte read from the device.
        uint8_t tmpBuf = 0;
        if (::read(i2c_file, &tmpBuf, 1) == -1)
            return -1;
        return tmpBuf;
    }

    /**
     * Read up to @p length bytes directly into @p buffer via a single syscall.
     *
     * Bypasses the RXbuf software buffer; intended for bulk transfers where
     * the caller manages its own buffering.
     */
    size_t LinuxHardwareI2C::readBytes(char *buffer, size_t length) {
        int bytes_read = 0;

        if (length == 0) {
            return length;
        } else {
            bytes_read = ::read(i2c_file, buffer, length);
            if (bytes_read < 0)
                bytes_read = 0;
        }
        return bytes_read;
    }

    /**
     * Return the number of bytes available to read.
     *
     * Checks the software RX buffer first; if empty, queries the kernel via
     * FIONREAD (bytes pending in the driver's receive queue).
     */
    int LinuxHardwareI2C::available() {
        // Same signed cast as in read(): prevents size_t underflow (latent bug fix).
        if (RXlen - (int)RXindex != 0)
            return RXlen - RXindex;
        int numBytes = 0;
        ioctl(i2c_file, FIONREAD, &numBytes);
        return numBytes;
    }

    /**
     * Read @p count bytes from @p address, optionally preceded by a register
     * write (combined transaction).
     *
     * If requestedBytes > 0 (i.e., write() was called since the last
     * beginTransmission()), issues a single ioctl(I2C_RDWR) with two i2c_msg
     * entries: a write message carrying the TX buffer (typically the register
     * address) followed by a read message — no STOP is generated between them.
     * This is required by most I²C sensors for register-addressed reads.
     *
     * If no TX bytes are pending, issues a plain ioctl(I2C_SLAVE) + read().
     *
     * See: https://stackoverflow.com/questions/505023/reading-writing-from-using-i2c-on-linux
     */
    uint8_t LinuxHardwareI2C::requestFrom(uint8_t address, size_t count, bool stop) {
        if (requestedBytes) {
            // Combined write-then-read: send TXbuf (e.g. register address)
            // then read @count bytes — all in one atomic I2C transaction.
            struct i2c_msg rdwr_msgs[2] = {
                {  // Write phase: send the register address / command bytes
                .addr = address,
                .flags = 0,  // write direction
                .len = (__u16)requestedBytes,
                .buf = (unsigned char *) TXbuf
                },
                {  // Read phase: receive sensor data into RXbuf
                .addr = address,
                .flags = I2C_M_RD,  // read direction
                .len = (__u16)count,
                .buf = (unsigned char *)RXbuf
                }
            };

            struct i2c_rdwr_ioctl_data rdwr_data = {
                .msgs = rdwr_msgs,
                .nmsgs = 2
            };

            int result = ioctl(i2c_file, I2C_RDWR, &rdwr_data);
            if (result >= 0) {
                RXlen = count;  // Mark buffer as holding @count valid bytes
                return count;
            } else {
                return result;  // Propagate negative errno
            }
        } else {
            // Simple read: set slave address and read directly.
            ioctl(i2c_file, I2C_SLAVE, address);
            RXlen = ::read(i2c_file, RXbuf, count);
            if (RXlen < 1) {
                RXlen = 0;
            }
            RXindex = 0;  // Reset read cursor for fresh buffer
            return RXlen;
        }
    }
}
