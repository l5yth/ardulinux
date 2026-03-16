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

// Casts for Radiolib HAL
#define RADIOLIB_ARDUINOHAL_PIN_MODE_CAST (PinMode)
#define RADIOLIB_ARDUINOHAL_PIN_STATUS_CAST (PinStatus)
#define RADIOLIB_ARDUINOHAL_INTERRUPT_MODE_CAST (PinStatus)

#include "ArduinoAPI.h"
#include <argp.h>
#if defined(__AVR__)
#include "avr/pgmspace.h"
#else
#include "deprecated-avr-comp/avr/pgmspace.h"
#endif
#ifdef __cplusplus

#include "linux/LinuxHardwareSPI.h"
#include "linux/LinuxSerial.h"
#ifdef ARDULINUX_LINUX_HARDWARE
#include "linux/LinuxHardwareI2C.h"
#else
#include "simulated/SimHardwareI2C.h"
#endif
#include <argp.h>

using namespace arduino;

typedef HardwareI2C TwoWire; // Some Arduino ports use this terminology

/** Map a pin number to an interrupt # 
 * We always map 1:1
*/
inline pin_size_t digitalPinToInterrupt(pin_size_t pinNumber) { return pinNumber; }

/** apps run under ardulinux can optionally define a ardulinuxSetup() to
 * use ardulinux specific init code (such as gpioBind) to setup ardulinux on
 * their host machine, before running 'arduino' code.
 * 
 * This function is called after ardulinuxCustomInit() (and after command line argument processing)
 */
extern void ardulinuxSetup();

/** Apps can optionally define this function to do *very* early app init.  typically you should just use it to call ardulinuxAddArguments()
 */
extern void ardulinuxCustomInit();

/**
 * call from portuinoCustomInit() if you want to add custom command line arguments
 */
void ardulinuxAddArguments(const struct argp_child &child, void *childArguments);

/**
 * write a 6 byte 'macaddr'/unique ID to the dmac parameter
 * This value can be customized with the --macaddr parameter and it defaults to 00:00:00:00:00:01
 */
void reboot();
#endif
