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

// result codes for endTransmission per Arduino docs
enum ResultI2c {
  I2cSuccess = 0,
  I2cTooLong,
  I2cAddrNAK,
  I2cDataNAK,
  I2cOtherError
};

class SimHardwareI2C : public HardwareI2C {
public:
  virtual void begin() NOT_IMPLEMENTED("i2cbegin");
  virtual int writeQuick(uint8_t toWrite) NOT_IMPLEMENTED("writeQuick");

  virtual void begin(uint8_t address) NOT_IMPLEMENTED("i2cslave begin");

  virtual void end() NOT_IMPLEMENTED("i2cend");

  virtual void setClock(uint32_t freq) NOT_IMPLEMENTED("i2csetClock");

  virtual void beginTransmission(uint8_t address) {
    // FIXME - implement
  }

  virtual uint8_t endTransmission(bool stopBit) {
    // notImplemented("i2cEndTransmission"); FIXME implement
    return I2cAddrNAK; // Claim everyone naks
  }

  virtual uint8_t endTransmission(void) { return endTransmission(true); }

  virtual uint8_t requestFrom(uint8_t address, size_t len, bool stopBit) {
    // notImplemented("requestFrom");
    return 0;
  }

  virtual uint8_t requestFrom(uint8_t address, size_t len) {
    // notImplemented("requestFrom");
    return 0;
  }

  virtual void onReceive(void (*)(int)) NOT_IMPLEMENTED("onReceive");

  virtual void onRequest(void (*)(void)) NOT_IMPLEMENTED("onRequest");

  // Methods from Print

  virtual size_t write(uint8_t) {
    notImplemented("writei2c");
    return 0;
  }

  virtual size_t write(const uint8_t *buffer, size_t size) {
    notImplemented("writeNi2c");
    return 0;
  }

  // Methods from Stream

  virtual int available() {
    notImplemented("i2cavailable");
    return 0;
  }

  virtual int read() {
    notImplemented("i2cread");
    return -1;
  }

  virtual int peek() {
    notImplemented("i2cpeek");
    return -1;
  }
};

extern SimHardwareI2C Wire;
} // namespace arduino

#endif // ARDULINUX_SIMHARDWAREI2C_H
