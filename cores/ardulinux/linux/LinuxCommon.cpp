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
#include "ArduLinuxGPIO.h"

#include <sched.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/**
 * Suspend execution for at least @p milliSec milliseconds.
 *
 * When real hardware is present, gpioIdle() is called before sleeping so that
 * ISRs are polled even during long delays.  usleep() is used (rather than
 * nanosleep) because it yields the CPU to other threads.
 *
 * @param milliSec Delay duration in milliseconds.
 */
void delay(unsigned long milliSec) {
  if (realHardware)
    gpioIdle();  // Poll ISRs so hardware events are not missed during delay
  usleep(milliSec * 1000);
}

/**
 * Suspend execution for at least @p usec microseconds.
 *
 * usleep() is preferred over nanosleep() because it allows other threads to
 * run during the wait.
 *
 * @param usec Delay duration in microseconds.
 */
void delayMicroseconds(unsigned int usec) {
  usleep(usec);
}

/** Cooperatively yield the CPU to other runnable threads. */
void yield(void) { sched_yield(); }

/** Return a random number in [0, max). */
long random(long max) { return random(0, max); }

/**
 * Return a random number in [@p min, @p max).
 *
 * Returns @p min immediately when the range is empty (min >= max) to avoid
 * division by zero in the modulo operation.
 */
long random(long min, long max) {
  if (min >= max) {
    return min;
  }
  return rand() % (max - min) + min;
}

/** Seed the pseudo-random number generator. @see random() */
void randomSeed(unsigned long s) { srand(s); }

/** Not implemented; Linux has no PWM tone API at this level. */
void tone(uint8_t _pin, unsigned int frequency, unsigned long duration)
    NOT_IMPLEMENTED("tone");

/** Not implemented. */
void noTone(uint8_t _pin) NOT_IMPLEMENTED("noTone");