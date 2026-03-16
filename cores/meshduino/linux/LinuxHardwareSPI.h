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

#pragma once

#include "HardwareSPI.h"
#include "SPIChip.h"
#include <memory>
#include <string>

namespace arduino {

class LinuxHardwareSPI : public HardwareSPI {
    std::shared_ptr<SPIChip> spiChip;
    std::string spiString;
    uint32_t defaultFreq = 2000000;

public:
    LinuxHardwareSPI(int8_t spi_host = 0);

    virtual uint8_t transfer(uint8_t data) override;
    virtual uint16_t transfer16(uint16_t data) override;
    virtual void transfer(void *buf, size_t count) override;

    // Meshduino extension: separate out/in buffers
    void transfer(void *out, void *in, size_t count);

    virtual void usingInterrupt(int interruptNumber) override;
    virtual void notUsingInterrupt(int interruptNumber) override;
    virtual void beginTransaction(SPISettings settings) override;
    virtual void endTransaction(void) override;

    virtual void attachInterrupt() override;
    virtual void detachInterrupt() override;

    virtual void begin() override;
    virtual void end() override;

    // Meshduino extensions: begin with explicit frequency or device path
    void begin(uint32_t freq);
    void begin(const char *name, uint32_t freq);
};

extern LinuxHardwareSPI SPI;

} // namespace arduino
