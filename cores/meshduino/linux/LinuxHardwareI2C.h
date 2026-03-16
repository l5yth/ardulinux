// Meshduino - Arduino API for Linux
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

#ifndef MESHDUINO_SIMHARDWAREI2C_H
#define MESHDUINO_SIMHARDWAREI2C_H

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

class LinuxHardwareI2C : public HardwareI2C {
  int i2c_file = 0;

public:
  void begin(const char* device);

  virtual void begin();

  virtual void begin(uint8_t address) {begin(); };

  virtual void end();

  virtual void setClock(uint32_t freq) NOT_IMPLEMENTED("i2csetClock");

  virtual void beginTransmission(uint8_t address);

  virtual uint8_t endTransmission(bool stopBit);

  virtual uint8_t endTransmission(void) { return endTransmission(true); }

  virtual uint8_t requestFrom(uint8_t address, size_t len, bool stopBit);

  virtual uint8_t requestFrom(uint8_t address, size_t len)  {
    return requestFrom(address, len, true);
  }

  virtual void onReceive(void (*)(int)) NOT_IMPLEMENTED("onReceive");

  virtual void onRequest(void (*)(void)) NOT_IMPLEMENTED("onRequest");

  // Methods from Print

  virtual size_t write(uint8_t toWrite);
  
  int writeQuick(uint8_t toWrite);

  virtual size_t write(const uint8_t *buffer, size_t size);

  // Methods from Stream

  virtual int available();

  virtual int read();

  virtual uint8_t readBytes(uint8_t address, size_t len, bool stopBit) {
    notImplemented("readBytes");
    return 0;
  }

  virtual size_t readBytes( char *buffer, size_t length);

  virtual int peek() {
    notImplemented("i2cpeek");
    return -1;
  }
};

extern LinuxHardwareI2C Wire;
} // namespace arduino

#endif // MESHDUINO_SIMHARDWAREI2C_H
