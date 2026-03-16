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

#ifndef ARDULINUX_UTILITY_H
#define ARDULINUX_UTILITY_H

#include <stdexcept>
#include <string>

class Exception: public std::runtime_error
{
public:

    /** Constructor (C++ STL strings).
     *  @param message The error message.
     */
    explicit Exception(const std::string& message)
        : runtime_error(message) {}

};

void notImplemented(const char *msg);

// Used as a suffix added after a method/function declaration
#define NOT_IMPLEMENTED(msg)                                                   \
  { notImplemented(msg); __builtin_unreachable(); }

/**
 * Normally arduino apps don't use exceptions.  If exceptions are allowed an exception will be thrown, otherwise an error message will be printed
 * and the function will return.
 */
[[noreturn]] void ardulinuxError(const char *msg, ...);

int ardulinuxCheckNotNeg(int result, const char *msg, ...);
int ardulinuxCheckZero(int result, const char *msg, ...);

/** Trigger a debugger breakpoint if in the debugger 
 */
void ardulinuxDebug();

// Compatibility aliases for libraries built against the portduino API
// (e.g. the upstream arduino-libraries/WiFi at the pinned submodule commit).
#define portduinoCheckNotNeg ardulinuxCheckNotNeg
#define portduinoCheckZero   ardulinuxCheckZero
#define portduinoDebug       ardulinuxDebug

#endif // ARDULINUX_UTILITY_H
