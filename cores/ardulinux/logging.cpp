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

#include "logging.h"
#include "Utility.h"

#include <Arduino.h>

namespace arduino {

void log(LogSystem sys, LogLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logv(sys, level, fmt, args);
  va_end(args);
}

void logv(LogSystem sys, LogLevel level, const char *fmt, va_list args) {
  // Fixed 256-byte stack buffer: messages longer than 255 chars are silently
  // truncated.  This is intentional to keep stack usage bounded; a future
  // improvement could use heap allocation with a fallback.
  char buf[256];
  vsnprintf(buf, sizeof(buf), fmt, args);
  // Route all log output through the Serial abstraction so that the logging
  // destination can be redirected by swapping the Serial implementation.
  Serial.write(buf);
  Serial.write('\n');
}

void log_e(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logv(SysCurrent, LogError, fmt, args);
  va_end(args);
}

void log_w(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logv(SysCurrent, LogWarn, fmt, args);
  va_end(args);
}

void log_i(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logv(SysCurrent, LogInfo, fmt, args);
  va_end(args);
}

void log_d(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logv(SysCurrent, LogDebug, fmt, args);
  va_end(args);
}

void log_v(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logv(SysCurrent, LogVerbose, fmt, args);
  va_end(args);
}

}