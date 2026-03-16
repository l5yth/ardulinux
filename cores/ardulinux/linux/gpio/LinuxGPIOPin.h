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

#ifdef ARDULINUX_HARDWARE

#include "Arduino.h"
#include "Common.h"
#include "ArduLinuxGPIO.h"
#include "Utility.h"
#include "logging.h"

#include <assert.h>
#include <stdlib.h>
#include <gpiod.h>

#ifndef GPIOD_LINE_BULK_MAX_LINES
#define GPIOD_V 2
#define GPIOD_LINE_REQUEST_DIRECTION_AS_IS GPIOD_LINE_DIRECTION_AS_IS
#define gpiod_line gpiod_line_request
#else
#define GPIOD_V 1
#endif

/**
 * Adapts the modern linux GPIO API for use by ArduLinux
 */
class LinuxGPIOPin : public GPIOPin {
  /// Our GPIO line
  struct gpiod_line *line;

  /// Chip structure associated with the line
  struct gpiod_chip *chip;

public:

  /**
    * Create a pin given a linux chip label and pin name
    */
  LinuxGPIOPin(pin_size_t n, const char *chipLabel, const char *linuxPinName, const char *ardulinuxPinName = NULL);
  LinuxGPIOPin(pin_size_t n, const char *chipLabel, const int linuxPinNum, const char *ardulinuxPinName);

  /**
   * Constructor
   * @param l is the low level linux GPIO pin object
   */
  // LinuxGPIOPin(pin_size_t n, String _name, struct gpiod_line *l)
  //    : GPIOPin(n, _name), line(l) {}

  ~LinuxGPIOPin();

protected:
  /// Read the low level hardware for this pin
  virtual PinStatus readPinHardware();
  virtual void writePin(PinStatus s);
  virtual void setPinMode(PinMode m);
  unsigned int offset;
private:
  gpiod_line *getLine(const char *chipLabel, const int linuxPinNum);
  gpiod_line *getLine(const char *chipLabel, const char *linuxPinName);

  #if GPIOD_V == 2
  #define gpiod_line_release gpiod_line_request_release
  int gpiod_line_get_value(gpiod_line_request *line){return gpiod_line_request_get_value(line, offset);}
  int gpiod_line_set_value(gpiod_line_request *line, PinStatus s){return gpiod_line_request_set_value(line, offset, (gpiod_line_value) s);}
  #endif
};

#endif
