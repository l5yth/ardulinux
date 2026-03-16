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

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include "LinuxHardwareI2C.h"

extern "C"
{
    #include <linux/i2c-dev.h>
    #include <i2c/smbus.h>
}


namespace arduino {
    char TXbuf[1000] = {0};
    char RXbuf[1000] = {0};
    int RXlen = 0;
    size_t RXindex = 0;
    int requestedBytes = 0;
    int sessionstatus;
    LinuxHardwareI2C Wire;
    bool hasBegun = false;

    void LinuxHardwareI2C::begin() {
        begin("/dev/i2c-1");
    }
    void LinuxHardwareI2C::begin(const char * device) {
        if (!hasBegun) {
            i2c_file = open(device, O_RDWR);
            hasBegun = true;
        }
    }
    void LinuxHardwareI2C::end() {
        if (hasBegun) {
            close(i2c_file);
            hasBegun = false;
        }
    }


    void LinuxHardwareI2C::beginTransmission(uint8_t address) {
        sessionstatus = ioctl(i2c_file, I2C_SLAVE, address);
        requestedBytes = 0;
    }
    uint8_t LinuxHardwareI2C::endTransmission(bool stopBit) {
        if (stopBit && requestedBytes) {
            int resp = ::write(i2c_file, TXbuf, requestedBytes);
            bool success = resp == requestedBytes;
            requestedBytes = 0;
            if (success)
                return 0;
            else
                return 4;
        }
        return 0;

    }

    int LinuxHardwareI2C::writeQuick(uint8_t toWrite) {
        int a = i2c_smbus_write_quick(i2c_file, toWrite);
        return a;
    }

    size_t LinuxHardwareI2C::write(uint8_t toWrite) {
        if (requestedBytes >= (int)sizeof(TXbuf))
            return 0;
        TXbuf[requestedBytes] = toWrite;
        requestedBytes++;
        return 1;
    }
    size_t LinuxHardwareI2C::write(const uint8_t *buffer, size_t size) {
        size_t written = 0;
        for (size_t i = 0; i < size; i++) {
            if (requestedBytes >= (int)sizeof(TXbuf))
                break;
            TXbuf[requestedBytes] = buffer[i];
            requestedBytes++;
            written++;
        }
        return written;
    }

    int LinuxHardwareI2C::read() {
        if (RXlen - RXindex != 0) {
            int tmpVal = RXbuf[RXindex];
            RXindex++;
            if (RXindex == RXlen || RXindex > 999) {
                RXindex = 0;
                RXlen = 0;
            }
            return tmpVal;
        }
        uint8_t tmpBuf = 0;
        if (::read(i2c_file, &tmpBuf, 1) == -1)
            return -1;
        return tmpBuf;
    }

    size_t LinuxHardwareI2C::readBytes(char *buffer, size_t length) {
        int bytes_read = 0;

        if (length == 0) {
            return length;
        } else {
            bytes_read = ::read(i2c_file, buffer, length);
            if ( bytes_read < 0)
                bytes_read = 0;
        }
        return bytes_read;
    }

    int LinuxHardwareI2C::available() {
        if (RXlen - RXindex != 0)
            return RXlen - RXindex;
        int numBytes = 0;
        ioctl(i2c_file, FIONREAD, &numBytes);
        return numBytes;
    }

    uint8_t LinuxHardwareI2C::requestFrom(uint8_t address, size_t count, bool stop) {
        // https://stackoverflow.com/questions/505023/reading-writing-from-using-i2c-on-linux
        // Need to do a combined write-read for some devices, so delay the write til then
        if (requestedBytes) {
            struct i2c_msg rdwr_msgs[2] = {
                {  // Start address
                .addr = address,
                .flags = 0, // write
                .len = (__u16)requestedBytes,
                .buf = (unsigned char *) TXbuf
                },
                { // Read buffer
                .addr = address,
                .flags = I2C_M_RD, // read
                .len = (__u16)count,
                .buf = (unsigned char *)RXbuf
                }
            };

            struct i2c_rdwr_ioctl_data rdwr_data = {
                .msgs = rdwr_msgs,
                .nmsgs = 2
            };

            int result = ioctl( i2c_file, I2C_RDWR, &rdwr_data );
            if (result >= 0) {
                RXlen = count;
                return count;
            } else {
                return result;
            }
        } else {
            ioctl(i2c_file, I2C_SLAVE, address);
            RXlen  = ::read(i2c_file, RXbuf, count);
            if (RXlen < 1) {
                RXlen = 0;
            }
            RXindex = 0;
            return RXlen;
        }
    }
}
