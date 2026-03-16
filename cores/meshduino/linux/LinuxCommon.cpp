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

#include "Common.h"
#include "Utility.h"
#include "MeshduinoGPIO.h"

#include <sched.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void delay(unsigned long milliSec) {
  //timespec ts{.tv_sec = (time_t)(milliSec / 1000),
  //            .tv_nsec = (long)(milliSec % 1000) * 1000L * 1000L};
  //nanosleep(&ts, NULL);
  if (realHardware)
    gpioIdle();
  usleep(milliSec * 1000); 
}

void delayMicroseconds(unsigned int usec) {
  usleep(usec); // better than nanosleep because it lets other threads run
}

void yield(void) { sched_yield(); }

long random(long max) { return random(0, max); }

long random(long min, long max) { 
  if (min >= max) {
    return min;
  }
  return rand() % (max - min) + min; 
}

void randomSeed(unsigned long s) { srand(s); }

void tone(uint8_t _pin, unsigned int frequency, unsigned long duration)
    NOT_IMPLEMENTED("tone");

void noTone(uint8_t _pin) NOT_IMPLEMENTED("noTone");