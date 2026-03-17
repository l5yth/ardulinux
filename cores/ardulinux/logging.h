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

#include <cstdarg>

namespace arduino {

/**
 * Log severity levels, ordered from least to most verbose.
 *
 * Higher values are noisier; a production build typically enables LogInfo
 * and above while debug builds enable LogDebug or LogVerbose.
 */
enum LogLevel { LogVerbose, LogDebug, LogInfo, LogWarn, LogError };

/**
 * Subsystem identifiers used to tag log messages.
 *
 * SysCurrent (0) reuses the last explicitly set subsystem, allowing a series
 * of related log calls to share a tag without repeating it.  Application
 * code should use IDs >= SysApp0 to avoid collisions with library subsystems.
 */
enum LogSystem {
  SysCurrent = 0, ///< Inherit the most recently used subsystem tag
  SysUnknown,     ///< Unclassified messages
  SysCore,        ///< Core ArduLinux runtime
  SysGPIO,        ///< GPIO subsystem
  SysI2C,         ///< I²C subsystem
  SysSPI,         ///< SPI subsystem
  SysInterrupt,   ///< Interrupt / ISR subsystem
  SysWifi,        ///< Wi-Fi subsystem

  SysApp0 = 1000  ///< First ID reserved for application-specific subsystems
};

/**
 * Log a formatted message from a specific subsystem.
 *
 * @param sys   Subsystem that produced the message (see LogSystem).
 * @param level Severity of the message (see LogLevel).
 * @param fmt   printf-style format string.
 * @param ...   Arguments for the format string.
 */
void log(LogSystem sys, LogLevel level, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/**
 * Log a formatted message using an already-expanded va_list.
 *
 * This is the common implementation target called by log() and the log_X()
 * convenience wrappers after they unpack their variadic arguments.
 *
 * @param sys   Subsystem tag.
 * @param level Severity level.
 * @param fmt   printf-style format string.
 * @param args  Caller-owned va_list (not modified by this function).
 *
 * @note Messages longer than 255 characters are silently truncated.  The
 *       fixed stack buffer is intentional to keep stack usage bounded; heap
 *       allocation is a possible future improvement.
 */
void logv(LogSystem sys, LogLevel level, const char *fmt, va_list args);

/** Log at LogError level using SysCurrent. @see log() */
void log_e(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
/** Log at LogWarn level using SysCurrent. @see log() */
void log_w(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
/** Log at LogInfo level using SysCurrent. @see log() */
void log_i(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
/** Log at LogDebug level using SysCurrent. @see log() */
void log_d(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
/** Log at LogVerbose level using SysCurrent. @see log() */
void log_v(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

}
