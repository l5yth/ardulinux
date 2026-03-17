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

#ifndef ARDULINUX_I2CTYPES_H
#define ARDULINUX_I2CTYPES_H

namespace arduino {

/**
 * Return codes for endTransmission(), matching the Arduino Wire API docs.
 *
 * Shared between SimHardwareI2C and LinuxHardwareI2C so application code
 * can check the result without caring which backend is in use.
 *
 *  - I2cSuccess   (0): transmission succeeded.
 *  - I2cTooLong   (1): data too long to fit in the transmit buffer.
 *  - I2cAddrNAK   (2): NACK received on address byte.
 *  - I2cDataNAK   (3): NACK received on data byte.
 *  - I2cOtherError(4): other error.
 */
enum ResultI2c {
  I2cSuccess = 0,  ///< Transmission succeeded
  I2cTooLong,      ///< TX buffer overflow
  I2cAddrNAK,      ///< Address phase not acknowledged
  I2cDataNAK,      ///< Data phase not acknowledged
  I2cOtherError    ///< Unclassified I²C error
};

} // namespace arduino

#endif // ARDULINUX_I2CTYPES_H
