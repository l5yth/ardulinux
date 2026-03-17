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

#include "Utility.h"
#include "AppInfo.h"
#include <csignal>
#include <stdio.h>
#include <stdarg.h>

void notImplemented(const char *msg) { printf("%s is not implemented\n", msg); }

void ardulinuxError(const char *msg, ...) {
  char msgBuffer[256];
  // Expand the printf-style format string into a fixed-size buffer before
  // throwing so that the Exception carries a fully rendered message.
  va_list args;
  va_start(args, msg);
  vsnprintf(msgBuffer, sizeof msgBuffer, msg, args);
  va_end(args);
  printf("%s critical error: %s\n", ardulinuxAppName, msgBuffer);
  throw Exception(msgBuffer);
}

int ardulinuxCheckNotNeg(int result, const char *msg, ...) {
  if (result < 0) {
    // Include errno so callers can diagnose OS-level failures without
    // needing to capture it themselves before calling this function.
    printf("%s notneg errno=%d: %s\n", ardulinuxAppName, errno, msg);
    throw Exception(msg);
  }
  return result;
}


int ardulinuxCheckZero(int result, const char *msg, ...) {
  if (result != 0) {
    printf("%s checkzero %d: %s\n", ardulinuxAppName, result, msg);
    throw Exception(msg);
  }
  return result;
}

void ardulinuxDebug() {
  // Raise SIGINT to pause execution in an attached debugger (e.g. gdb).
  // When not debugging, this terminates the process unless the signal is
  // handled elsewhere.
  std::raise(SIGINT);
}
