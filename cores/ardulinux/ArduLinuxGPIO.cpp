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

#include "Common.h"
#include "Utility.h"
#include "logging.h"
#include "Arduino.h"
#include "ArduLinuxGPIO.h"

#include <assert.h>
#include <memory>
#include <stdlib.h>
#include <vector>

// #include "linux/gpio/classic/GPIOChip.h"

/** Total number of GPIO slots; set by gpioInit(). */
int NUM_GPIOS;

/** True once at least one real hardware pin has been bound via gpioBind(). */
bool realHardware = false;

/** Indexed array of all GPIO pin implementations (owned pointers). */
std::vector<std::unique_ptr<GPIOPinIf>> pins;

GPIOPinIf::~GPIOPinIf() {}

/**
 * Populate the pin table with SimGPIOPin instances.
 *
 * Note: appends to the existing vector rather than resetting it, so calling
 * this function more than once grows the table unboundedly.  Call only once
 * per process lifetime.
 */
void gpioInit(int _num_gpios) {
  NUM_GPIOS = _num_gpios;
  for(size_t i = 0; i < NUM_GPIOS; i++)
    pins.push_back(std::make_unique<SimGPIOPin>(i, "Unbound"));
}

/** Poll every pin that has an ISR attached and fire the ISR if triggered. */
void gpioIdle() {
  for(size_t i = 0; i < NUM_GPIOS; i++)
    pins[i]->refreshIfNeeded();
}

/**
 * Install a real hardware pin at its declared pin number.
 *
 * Destroys the SimGPIOPin that previously occupied the slot and replaces it
 * with @p p.  Setting realHardware=true causes the main loop to skip its
 * 100 ms idle sleep so that ISR polling happens at full throughput.
 */
void gpioBind(GPIOPinIf *p) {
  if (!(p->getPinNum() < NUM_GPIOS))
    log(SysGPIO, LogError, "getPinNum %u isn't smaller than max %d", p->getPinNum(), NUM_GPIOS);
  assert(p->getPinNum() < NUM_GPIOS);
  pins[p->getPinNum()].reset(p);
  realHardware = true; // Disable loop sleep now that real hardware is present
}

/**
 * Look up and return the pin implementation for Arduino pin @p n.
 *
 * Aborts (via assert) if @p n is out of range.  Internal use only; Arduino
 * API functions (digitalRead, digitalWrite, etc.) call this.
 *
 * @param n Arduino pin number.
 * @return Pointer to the GPIOPinIf implementation (never NULL).
 */
GPIOPinIf *getGPIO(pin_size_t n)
{
  if (!(n < NUM_GPIOS))
    log(SysGPIO, LogError, "Pin %u isn't smaller than max %d", n, NUM_GPIOS);
  assert(n < NUM_GPIOS);
  return pins[n].get();
}

/** Arduino API: configure the direction/mode of a digital pin. */
void pinMode(pin_size_t pinNumber, PinMode pinMode)
{
  auto p = getGPIO(pinNumber);
  p->setPinMode(pinMode);
}

/** Arduino API: write HIGH or LOW to a digital pin. */
void digitalWrite(pin_size_t pinNumber, PinStatus status)
{
  auto p = getGPIO(pinNumber);
  p->writePin(status);
}

/** Arduino API: read the current logic level of a digital pin. */
PinStatus digitalRead(pin_size_t pinNumber)
{
  auto p = getGPIO(pinNumber);
  return p->readPin();
}

/** Arduino API: measure the duration of a pulse on a pin. */
unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout)
{
  auto p = getGPIO(pin);
  return p->pulseIn((PinStatus)state, timeout);
}

/** Arduino API: read an analog value from a pin (not widely supported). */
int analogRead(pin_size_t pinNumber)
{
  auto p = getGPIO(pinNumber);
  auto r = p->analogRead();
  log(SysGPIO, LogDebug, "analogRead(%d) -> %d", pinNumber, r);
  return r;
}

/** Arduino API: write a PWM/analog value to a pin (not widely supported). */
void analogWrite(pin_size_t pinNumber, int value)
{
  auto p = getGPIO(pinNumber);
  log(SysGPIO, LogDebug, "analogWrite(%d) -> %d", pinNumber, value);
  p->analogWrite(value);
}

/** Arduino API: attach an ISR callback to a pin, triggered by @p mode. */
void attachInterrupt(pin_size_t interruptNumber, voidFuncPtr callback,
                     PinStatus mode)
{
  auto p = getGPIO(interruptNumber);
  p->attachInterrupt(callback, mode);
}

/** Arduino API: remove a previously attached ISR from a pin. */
void detachInterrupt(pin_size_t interruptNumber)
{
  auto p = getGPIO(interruptNumber);
  p->detachInterrupt();
}
