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

#ifndef ARDULINUX_LINUXHARDWAREI2C_H
#define ARDULINUX_LINUXHARDWAREI2C_H

#include "HardwareI2C.h"
#include "Utility.h"
#include "I2CTypes.h"

namespace arduino {

/**
 * Arduino Wire (I²C master) implementation using the Linux i2c-dev interface.
 *
 * Communicates via /dev/i2c-N character devices using ioctl(I2C_SLAVE) for
 * simple reads/writes and ioctl(I2C_RDWR) for combined write-then-read
 * transactions (required by many sensors).
 *
 * Uses a pair of module-level buffers (TXbuf/RXbuf) and associated index
 * variables declared in LinuxHardwareI2C.cpp.  Not thread-safe.
 */
class LinuxHardwareI2C : public HardwareI2C {
  int i2c_file = 0; ///< File descriptor for the open i2c-dev device

public:
  /**
   * Open a specific i2c-dev device.
   *
   * @param device Path to the i2c-dev node (e.g. "/dev/i2c-1").
   */
  void begin(const char* device);

  /** Open the default I²C device (/dev/i2c-1). */
  virtual void begin();

  /** Slave-mode begin (not applicable for master-only; delegates to begin()). */
  virtual void begin(uint8_t address) { begin(); }

  /** Close the i2c-dev file descriptor. */
  virtual void end();

  /** Not implemented; the Linux i2c-dev driver manages the clock. */
  virtual void setClock(uint32_t freq) NOT_IMPLEMENTED("i2csetClock");

  /**
   * Select the target device for the next write transaction.
   *
   * Sets the I2C_SLAVE target via ioctl and resets the TX buffer index.
   *
   * @param address 7-bit I²C device address.
   */
  virtual void beginTransmission(uint8_t address);

  /**
   * Flush the TX buffer to the device.
   *
   * When @p stopBit is true and bytes are pending, issues a write() syscall
   * to transmit the buffered bytes.
   *
   * @param stopBit If true, send the buffered bytes and generate a STOP.
   *                If false, the bytes remain buffered (for a restart).
   * @return ResultI2c code (0 = success, 4 = error).
   */
  virtual uint8_t endTransmission(bool stopBit);

  /** Flush and send a STOP condition (stopBit = true). */
  virtual uint8_t endTransmission(void) { return endTransmission(true); }

  /**
   * Read @p len bytes from device @p address into the RX buffer.
   *
   * If bytes are pending in the TX buffer (from a preceding write()), issues
   * a combined write-then-read transaction via ioctl(I2C_RDWR) to avoid
   * sending an intermediate STOP between the register address write and the
   * data read.  Otherwise issues a plain read().
   *
   * @param address 7-bit I²C device address.
   * @param len     Number of bytes to read.
   * @param stopBit Passed to the underlying I²C transaction (currently unused
   *                in the combined path).
   * @return Number of bytes received, or a negative error code.
   */
  virtual uint8_t requestFrom(uint8_t address, size_t len, bool stopBit);

  /** Request @p len bytes from @p address with implicit STOP. */
  virtual uint8_t requestFrom(uint8_t address, size_t len) {
    return requestFrom(address, len, true);
  }

  /** Not implemented (slave-mode callback; this is a master-only driver). */
  virtual void onReceive(void (*)(int)) NOT_IMPLEMENTED("onReceive");
  /** Not implemented (slave-mode callback; this is a master-only driver). */
  virtual void onRequest(void (*)(void)) NOT_IMPLEMENTED("onRequest");

  /**
   * Buffer one byte for transmission.
   *
   * The byte is added to TXbuf; the actual write happens in endTransmission()
   * or requestFrom() (combined write-read).
   *
   * @param toWrite Byte to queue.
   * @return 1 on success, 0 if the TX buffer is full.
   */
  virtual size_t write(uint8_t toWrite);

  /**
   * Issue an SMBus Quick Command (I²C address probe).
   *
   * @param toWrite Read/write bit to send (I2C_SMBUS_WRITE or I2C_SMBUS_READ).
   * @return 0 on success, negative on error.
   */
  int writeQuick(uint8_t toWrite);

  /**
   * Buffer multiple bytes for transmission.
   *
   * @param buffer Bytes to queue.
   * @param size   Number of bytes.
   * @return Number of bytes actually buffered.
   */
  virtual size_t write(const uint8_t *buffer, size_t size);

  /**
   * Return the number of bytes available to read from the RX buffer.
   *
   * Falls back to ioctl(FIONREAD) when the software buffer is empty.
   */
  virtual int available();

  /**
   * Read one byte from the RX buffer.
   *
   * If the software buffer (populated by requestFrom()) has data, returns
   * from there.  Otherwise issues a raw 1-byte read() syscall.
   *
   * @return Byte value (0–255), or -1 on error or no data.
   */
  virtual int read();

  /** Not implemented (not applicable for Linux i2c-dev). */
  virtual uint8_t readBytes(uint8_t address, size_t len, bool stopBit) {
    notImplemented("readBytes");
    return 0;
  }

  /**
   * Read up to @p length bytes directly into @p buffer via a single syscall.
   *
   * @param buffer Destination buffer.
   * @param length Maximum bytes to read.
   * @return Number of bytes actually read (0 on error).
   */
  virtual size_t readBytes(char *buffer, size_t length);

  /** Not implemented (no peek support in i2c-dev). */
  virtual int peek() {
    notImplemented("i2cpeek");
    return -1;
  }
};

/** Global I²C master instance; opened on /dev/i2c-1 by default. */
extern LinuxHardwareI2C Wire;
} // namespace arduino

#endif // ARDULINUX_LINUXHARDWAREI2C_H
