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

class SPIChip 
{
public:
    /**
     * Do a SPI transaction to the selected device
     * 
     * @param outBuf if NULL it will be not used (zeros will be sent)
     * @param inBuf if NULL it will not be used (device response bytes will be discarded)
     * @param deassertCS after last transaction (if not set, it will be left asserted)
     * @return 0 for success, else ERRNO fault code
     */
    virtual int transfer(const uint8_t *outBuf, uint8_t *inBuf, size_t bufLen, bool deassertCS = true) = 0;

    /// is this chip controlling real hardware?
    virtual const bool isSimulated() { return false; }
    virtual void beginTransaction(uint32_t clockSpeed) {};
    virtual void endTransaction() {};
};


class SimSPIChip : public SPIChip
{
public:
    /**
     * Do a SPI transaction to the selected device
     * 
     * @param outBuf if NULL it will be not used (zeros will be sent)
     * @param inBuf if NULL it will not be used (device response bytes will be discarded)
     * @param deassertCS after last transaction (if not set, it will be left asserted)
     * @return 0 for success, else ERRNO fault code
     */
    int transfer(const uint8_t *outBuf, uint8_t *inBuf, size_t bufLen, bool deassertCS = true) {
        // log(SysSPI, LogVerbose, "SIM: spiTransfer(%d) -> %d", bufLen);
        return 0;
    }

    /// is this chip controlling real hardware?
    virtual const bool isSimulated() { return true; }    
};

#endif
