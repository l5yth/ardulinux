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

#pragma once

#include <cstdarg>

namespace arduino {

// FIXME - move somewhere else and add gcc arg type hints

enum LogLevel { LogVerbose, LogDebug, LogInfo, LogWarn, LogError };

enum LogSystem {
  SysCurrent = 0, // The last set current subsystem
  SysUnknown,
  SysCore,
  SysGPIO,
  SysI2C,
  SysSPI,
  SysInterrupt,
  SysWifi,

  // Ids greater than 1000 are used for application specific purposes
  SysApp0 = 1000
};

void log(LogSystem sys, LogLevel level, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));
void logv(LogSystem sys, LogLevel level, const char *fmt, va_list args);

void log_e(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void log_w(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void log_i(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void log_d(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void log_v(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

}