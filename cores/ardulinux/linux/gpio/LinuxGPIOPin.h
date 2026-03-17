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

/**
 * Detect libgpiod API version at compile time.
 *
 * gpiod v1 defines GPIOD_LINE_BULK_MAX_LINES; gpiod v2 does not.
 * GPIOD_V is set to 1 or 2 accordingly, and a compatibility alias
 * is provided so the rest of the code can use the v1 type names.
 */
#ifndef GPIOD_LINE_BULK_MAX_LINES
#define GPIOD_V 2
/** gpiod v2 renamed this constant; alias it to keep the v1 name usable. */
#define GPIOD_LINE_REQUEST_DIRECTION_AS_IS GPIOD_LINE_DIRECTION_AS_IS
/** gpiod v2 uses gpiod_line_request instead of gpiod_line for the handle. */
#define gpiod_line gpiod_line_request
#else
#define GPIOD_V 1
#endif

/**
 * Arduino GPIO pin backed by the Linux libgpiod character-device API.
 *
 * Supports both libgpiod v1 and v2 (selected at compile time via GPIOD_V).
 * Lines are identified either by name (string lookup on the chip) or by
 * numeric offset within the chip.
 *
 * ISRs are not interrupt-driven; they are polled by gpioIdle() from the main
 * loop.  Use refreshIfNeeded() / attachInterrupt() as usual.
 */
class LinuxGPIOPin : public GPIOPin {
  gpiod_line *line;   ///< Acquired GPIO line handle (type aliased for v1/v2)
  gpiod_chip *chip;   ///< GPIO chip handle (kept open for reconfiguration)

public:

  /**
   * Construct from a chip label and a named GPIO line.
   *
   * @param n               Arduino pin number to assign.
   * @param chipLabel       Label of the gpiochip device (e.g. "pinctrl-bcm2835").
   *                        If a /dev/<chipLabel> path exists, it is opened
   *                        directly; otherwise all chips are scanned.
   * @param linuxPinName    Name of the GPIO line within the chip.
   * @param ardulinuxPinName Human-readable name for log messages (defaults to
   *                        linuxPinName if NULL).
   */
  LinuxGPIOPin(pin_size_t n, const char *chipLabel, const char *linuxPinName, const char *ardulinuxPinName = NULL);

  /**
   * Construct from a chip label and a numeric GPIO line offset.
   *
   * @param n               Arduino pin number to assign.
   * @param chipLabel       Label of the gpiochip device.
   * @param linuxPinNum     Zero-based offset of the GPIO line within the chip.
   * @param ardulinuxPinName Human-readable name for log messages.
   */
  LinuxGPIOPin(pin_size_t n, const char *chipLabel, const int linuxPinNum, const char *ardulinuxPinName);

  /** Release the GPIO line and close the chip. */
  ~LinuxGPIOPin();

protected:
  /** Read the current hardware pin level via gpiod_line_get_value(). */
  virtual PinStatus readPinHardware();

  /**
   * Write a logic level to the pin, auto-switching to OUTPUT mode if needed.
   *
   * Some libraries omit the pinMode(OUTPUT) call; this method silently
   * promotes the pin to output to avoid a silent no-op.
   */
  virtual void writePin(PinStatus s);

  /**
   * Reconfigure the GPIO line direction and bias.
   *
   * Uses gpiod_line_release + gpiod_line_request_* (v1) or
   * gpiod_line_request_reconfigure_lines (v2).
   */
  virtual void setPinMode(PinMode m);

  unsigned int offset; ///< Line offset within the chip (used by gpiod v2)

private:
  /**
   * Locate and acquire a GPIO line by numeric offset.
   *
   * @param chipLabel    gpiochip label or device name.
   * @param linuxPinNum  Line offset within the chip.
   * @return Acquired line handle; throws std::invalid_argument on failure.
   */
  gpiod_line *getLine(const char *chipLabel, const int linuxPinNum);

  /**
   * Locate and acquire a GPIO line by name.
   *
   * @param chipLabel    gpiochip label or device name.
   * @param linuxPinName Line name as reported by the kernel.
   * @return Acquired line handle; throws std::invalid_argument on failure.
   */
  gpiod_line *getLine(const char *chipLabel, const char *linuxPinName);

  /** @defgroup gpiod_v2_compat gpiod v2 compatibility shims
   *
   * gpiod v2 replaced the gpiod_line / gpiod_line_request split with a
   * unified gpiod_line_request handle.  These inline wrappers restore the
   * v1-style function signatures so the method bodies compile against both
   * library versions without #if guards scattered throughout.
   * @{
   */
  #if GPIOD_V == 2
  /** Alias: gpiod v2 renamed gpiod_line_release to gpiod_line_request_release. */
  #define gpiod_line_release gpiod_line_request_release
  /** Read the line value from a v2 request handle using the stored offset. */
  int gpiod_line_get_value(gpiod_line_request *line) { return gpiod_line_request_get_value(line, offset); }
  /** Write a logic level to a v2 request handle using the stored offset. */
  int gpiod_line_set_value(gpiod_line_request *line, PinStatus s) { return gpiod_line_request_set_value(line, offset, (gpiod_line_value) s); }
  #endif
  /** @} */
};

#endif
