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

// Stub definitions for arduino::HardwareSPI virtual methods.
// ArduinoCore-API declares these as non-pure virtual but provides no .cpp,
// expecting each platform to supply implementations. We provide stubs here so
// the linker can build the vtable; the concrete LinuxHardwareSPI class
// overrides all of them.

#include "HardwareSPI.h"

namespace arduino {

uint8_t HardwareSPI::transfer(uint8_t) { return 0; }
uint16_t HardwareSPI::transfer16(uint16_t) { return 0; }
void HardwareSPI::transfer(void *, size_t) {}
void HardwareSPI::usingInterrupt(int) {}
void HardwareSPI::notUsingInterrupt(int) {}
void HardwareSPI::beginTransaction(SPISettings) {}
void HardwareSPI::endTransaction() {}
void HardwareSPI::attachInterrupt() {}
void HardwareSPI::detachInterrupt() {}
void HardwareSPI::begin() {}
void HardwareSPI::end() {}

} // namespace arduino
