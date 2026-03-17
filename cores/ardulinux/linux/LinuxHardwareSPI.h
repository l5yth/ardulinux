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

#pragma once

#include "HardwareSPI.h"
#include "SPIChip.h"
#include <memory>
#include <string>

namespace arduino {

/**
 * Arduino HardwareSPI implementation backed by a Linux spidev device.
 *
 * Holds a shared_ptr<SPIChip> that is either a LinuxSPIChip (when
 * ARDULINUX_HARDWARE is defined and a spidev node is accessible) or a
 * SimSPIChip (no-op fallback).  The concrete chip is selected lazily on
 * begin().
 *
 * The default device is derived from the spi_host constructor argument:
 *   low nibble → bus number, high nibble → device number
 *   e.g. spi_host=0 → /dev/spidev0.0
 *
 * Applications can override the device path by calling begin(name, freq).
 */
class LinuxHardwareSPI : public HardwareSPI {
    std::shared_ptr<SPIChip> spiChip; ///< Active chip (real or simulated)
    std::string spiString;            ///< spidev device path, e.g. /dev/spidev0.0
    uint32_t defaultFreq = 2000000;   ///< Default clock frequency in Hz

public:
    /**
     * Construct with an optional spi_host encoding.
     *
     * @param spi_host Low nibble = bus index, high nibble = device index.
     *                 Default 0 → /dev/spidev0.0.
     */
    LinuxHardwareSPI(int8_t spi_host = 0);

    /** Arduino API: single-byte full-duplex transfer; returns MISO byte. */
    virtual uint8_t transfer(uint8_t data) override;

    /** Not implemented; always returns 0. */
    virtual uint16_t transfer16(uint16_t data) override;

    /** Arduino API: in-place buffer transfer (MOSI == MISO buffer). */
    virtual void transfer(void *buf, size_t count) override;

    /**
     * ArduLinux extension: full-duplex transfer with separate TX and RX
     * buffers.
     *
     * @param out  Transmit buffer (MOSI).
     * @param in   Receive buffer (MISO).
     * @param count Number of bytes to transfer.
     */
    void transfer(void *out, void *in, size_t count);

    /** Arduino API: no-op (interrupt interaction not applicable on Linux). */
    virtual void usingInterrupt(int interruptNumber) override;
    /** Arduino API: no-op. */
    virtual void notUsingInterrupt(int interruptNumber) override;

    /**
     * Arduino API: lock the bus and apply SPI settings for the transaction.
     *
     * Only SPI_MODE0 / MSBFIRST are supported; other modes trigger an assert.
     */
    virtual void beginTransaction(SPISettings settings) override;

    /** Arduino API: release the bus and restore the default clock speed. */
    virtual void endTransaction(void) override;

    /** Arduino API: no-op. */
    virtual void attachInterrupt() override;
    /** Arduino API: no-op. */
    virtual void detachInterrupt() override;

    /** Arduino API: open the spidev device using the default frequency. */
    virtual void begin() override;

    /** Arduino API: close and release the spidev device. */
    virtual void end() override;

    /**
     * ArduLinux extension: open the spidev device with an explicit frequency.
     *
     * @param freq Clock frequency in Hz.
     */
    void begin(uint32_t freq);

    /**
     * ArduLinux extension: open a specific spidev device path.
     *
     * @param name  spidev device path (e.g. "/dev/spidev1.0").  If NULL the
     *              previously configured path is used.
     * @param freq  Clock frequency in Hz.
     */
    void begin(const char *name, uint32_t freq);
};

/** Global SPI instance; maps to /dev/spidev0.0 by default. */
extern LinuxHardwareSPI SPI;

} // namespace arduino
