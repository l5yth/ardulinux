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

#include "HardwareSPI.h"
#ifndef SPIChip_DEFINED
#define SPIChip_DEFINED

/**
 * Abstract interface for a SPI chip (real or simulated).
 *
 * LinuxHardwareSPI holds a shared_ptr<SPIChip> so it can transparently swap
 * between the real spidev-backed LinuxSPIChip and the no-op SimSPIChip
 * without changing application code.
 */
class SPIChip
{
public:
    /**
     * Perform a full-duplex SPI transfer.
     *
     * Either buffer may be NULL:
     *  - NULL @p outBuf sends zeros on MOSI.
     *  - NULL @p inBuf discards MISO bytes.
     *
     * @param outBuf    Data to transmit (MOSI), or NULL to send zeros.
     * @param inBuf     Buffer to receive data (MISO), or NULL to discard.
     * @param bufLen    Number of bytes to transfer.
     * @param deassertCS If true, deassert chip-select after the transfer.
     * @return 0 on success, or a negative errno value on failure.
     */
    virtual int transfer(const uint8_t *outBuf, uint8_t *inBuf, size_t bufLen, bool deassertCS = true) = 0;

    /** @return true if this chip does not control real hardware. */
    virtual const bool isSimulated() { return false; }

    /**
     * Lock the SPI bus and set the clock speed for the upcoming transaction.
     *
     * @param clockSpeed Clock frequency in Hz.
     */
    virtual void beginTransaction(uint32_t clockSpeed) {};

    /** Release the SPI bus after a transaction. */
    virtual void endTransaction() {};
};


/**
 * No-op SPI chip implementation used when no spidev device is available.
 *
 * All transfers succeed immediately and return zeros in @p inBuf.
 * This allows application code to compile and run on hosts without SPI
 * hardware; only the SPI interaction is skipped.
 */
class SimSPIChip : public SPIChip
{
public:
    /**
     * Simulate a SPI transfer — does nothing, returns success.
     *
     * @param outBuf    Ignored.
     * @param inBuf     Left unchanged (caller receives whatever it contained).
     * @param bufLen    Ignored.
     * @param deassertCS Ignored.
     * @return Always 0 (success).
     */
    int transfer(const uint8_t *outBuf, uint8_t *inBuf, size_t bufLen, bool deassertCS = true) {
        return 0;
    }

    /** @return Always true — this chip does not control real hardware. */
    virtual const bool isSimulated() { return true; }
};

#endif
