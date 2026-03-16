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

/** Absolute microsecond timestamp captured on the first micros() call. */
static unsigned long startUsec;

/**
 * Return microseconds elapsed since the first call to micros().
 *
 * Uses gettimeofday() for portability.  On the first call, startUsec is
 * initialised to the current absolute time so that subsequent calls return a
 * value relative to program start rather than the Unix epoch.
 *
 * @return Elapsed microseconds (wraps at ULONG_MAX ≈ 4295 seconds on 32-bit).
 */
extern "C" unsigned long micros(void) {
  struct timeval te;
  gettimeofday(&te, NULL);
  // Combine seconds and microseconds into a single 64-bit value to avoid
  // overflow before the subtraction.
  unsigned long long usecs = te.tv_sec * 1000000LL + te.tv_usec;
  if (startUsec == 0)      // One-time initialisation on first call
    startUsec = usecs;
  return usecs - startUsec;
}

/** Absolute millisecond timestamp captured on the first millis() call. */
static unsigned long startMsec;

/**
 * Return milliseconds elapsed since the first call to millis().
 *
 * Uses gettimeofday() for portability.  The start time is captured lazily on
 * the first call (same pattern as micros()).
 *
 * @return Elapsed milliseconds (wraps at ULONG_MAX ≈ 49 days on 32-bit).
 */
extern "C" unsigned long millis(void) {
  struct timeval te;
  gettimeofday(&te, NULL);
  // Convert to milliseconds: seconds * 1000 + fractional microseconds / 1000.
  unsigned long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;
  if (startMsec == 0)      // One-time initialisation on first call
    startMsec = milliseconds;
  return milliseconds - startMsec;
}
