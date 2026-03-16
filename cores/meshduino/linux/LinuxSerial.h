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

#ifndef MESHDUINO_LINUXSERIAL_H
#define MESHDUINO_LINUXSERIAL_H

#include "HardwareSerial.h"
#include <string>

namespace arduino {
    class LinuxSerial : public HardwareSerial {
        int serial_port = -1;
        std::string path;
    public:
        virtual void begin(unsigned long baudrate) { begin(baudrate, SERIAL_8N1 ); }
        virtual void begin(unsigned long baudrate, uint16_t config);
        virtual int setPath(std::string serialPath);
        virtual void end();
        virtual int available(void);
        virtual int peek(void);
        virtual int read(void);
        virtual void flush(void);
        virtual size_t write(uint8_t);
        using Print::write; // pull in write(str) and write(buf, size) from Print
        virtual operator bool();
    };

        class SimSerial : public HardwareSerial {
    public:
        virtual void begin(unsigned long baudrate) { begin(baudrate, SERIAL_8N1 ); }
        virtual void begin(unsigned long baudrate, uint16_t config);
        virtual void end();
        virtual int available(void);
        virtual int peek(void);
        virtual int read(void);
        virtual void flush(void);
        virtual size_t write(uint8_t);
        using Print::write; // pull in write(str) and write(buf, size) from Print
        virtual operator bool();
    };

    extern LinuxSerial Serial1;
    extern SimSerial Serial;
}

#endif //MESHDUINO_LINUXSERIAL_H
