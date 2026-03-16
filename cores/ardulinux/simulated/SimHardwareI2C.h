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

#ifndef ARDULINUX_SIMHARDWAREI2C_H
#define ARDULINUX_SIMHARDWAREI2C_H

#include "HardwareI2C.h"
#include "Utility.h"

namespace arduino {

/**
 * Return codes for endTransmission(), matching the Arduino Wire API docs.
 *
 * Shared between SimHardwareI2C and LinuxHardwareI2C so application code
 * can check the result without caring which backend is in use.
 */
enum ResultI2c {
  I2cSuccess = 0,  ///< Transmission succeeded
  I2cTooLong,      ///< TX buffer overflow
  I2cAddrNAK,      ///< Address phase not acknowledged
  I2cDataNAK,      ///< Data phase not acknowledged
  I2cOtherError    ///< Unclassified I²C error
};

/**
 * No-op I²C master implementation used when ARDULINUX_HARDWARE is not defined.
 *
 * All read/write operations call notImplemented() (which logs a message but
 * does not abort), and endTransmission() returns I2cAddrNAK to signal that no
 * device was found.  This lets application code compile and run on hosts
 * without I²C hardware; only I²C interactions are skipped.
 */
class SimHardwareI2C : public HardwareI2C {
public:
  /** Not implemented (no hardware). */
  virtual void begin() NOT_IMPLEMENTED("i2cbegin");
  /** Not implemented. */
  virtual int writeQuick(uint8_t toWrite) NOT_IMPLEMENTED("writeQuick");
  /** Not implemented (slave mode not applicable). */
  virtual void begin(uint8_t address) NOT_IMPLEMENTED("i2cslave begin");
  /** Not implemented. */
  virtual void end() NOT_IMPLEMENTED("i2cend");
  /** Not implemented. */
  virtual void setClock(uint32_t freq) NOT_IMPLEMENTED("i2csetClock");

  /** Accept address selection silently (no-op in simulation). */
  virtual void beginTransmission(uint8_t address) {}

  /**
   * Return I2cAddrNAK to indicate no device is present in simulation.
   *
   * @return I2cAddrNAK (2) always.
   */
  virtual uint8_t endTransmission(bool stopBit) {
    return I2cAddrNAK;
  }

  /** Delegate to endTransmission(true). */
  virtual uint8_t endTransmission(void) { return endTransmission(true); }

  /** Return 0 (no bytes received in simulation). */
  virtual uint8_t requestFrom(uint8_t address, size_t len, bool stopBit) {
    return 0;
  }

  /** Return 0 (no bytes received in simulation). */
  virtual uint8_t requestFrom(uint8_t address, size_t len) {
    return 0;
  }

  /** Not implemented (slave mode only). */
  virtual void onReceive(void (*)(int)) NOT_IMPLEMENTED("onReceive");
  /** Not implemented (slave mode only). */
  virtual void onRequest(void (*)(void)) NOT_IMPLEMENTED("onRequest");

  /** Not implemented; returns 0. */
  virtual size_t write(uint8_t) {
    notImplemented("writei2c");
    return 0;
  }

  /** Not implemented; returns 0. */
  virtual size_t write(const uint8_t *buffer, size_t size) {
    notImplemented("writeNi2c");
    return 0;
  }

  /** Not implemented; returns 0. */
  virtual int available() {
    notImplemented("i2cavailable");
    return 0;
  }

  /** Not implemented; returns -1. */
  virtual int read() {
    notImplemented("i2cread");
    return -1;
  }

  /** Not implemented; returns -1. */
  virtual int peek() {
    notImplemented("i2cpeek");
    return -1;
  }
};

/** Global Wire instance used when ARDULINUX_HARDWARE is not defined. */
extern SimHardwareI2C Wire;
} // namespace arduino

#endif // ARDULINUX_SIMHARDWAREI2C_H
