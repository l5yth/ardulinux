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

#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>

static unsigned long startUsec;

extern "C" unsigned long micros(void) {
  struct timeval te;
  gettimeofday(&te, NULL);                                  // get current time
  unsigned long long  usecs = te.tv_sec * 1000000LL + te.tv_usec; // calculate
  if (startUsec == 0)
    startUsec = usecs;

  return usecs - startUsec;
}

static unsigned long startMsec;

/**
 * Return msecs since this 'arduino' instance started running
 */
extern "C" unsigned long millis(void) {
  struct timeval te;
  gettimeofday(&te, NULL); // get current time

  unsigned long milliseconds =
      te.tv_sec * 1000LL + te.tv_usec / 1000; // calculate milliseconds

  if(startMsec == 0) // First run
    startMsec = milliseconds;

  return milliseconds - startMsec;
}
