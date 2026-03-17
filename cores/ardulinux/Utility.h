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

/**
 * Base exception type for ArduLinux runtime errors.
 *
 * Wraps std::runtime_error so callers can catch either ArduLinux-specific
 * errors or the broader std::runtime_error family.
 */
class Exception: public std::runtime_error
{
public:

    /** Constructor (C++ STL strings).
     *  @param message The error message.
     */
    explicit Exception(const std::string& message)
        : runtime_error(message) {}

};

/**
 * Print a "not implemented" message to stdout.
 *
 * Intended for use inside NOT_IMPLEMENTED() — do not call directly.
 * @param msg Human-readable identifier of the unimplemented feature.
 */
void notImplemented(const char *msg);

/**
 * Method/function body that prints a "not implemented" message and aborts.
 *
 * Usage: append as the body of a declared-but-not-yet-implemented method so
 * the compiler does not warn about missing returns while the intent is clear.
 *
 * Example:
 * @code
 *   virtual void setClock(uint32_t freq) NOT_IMPLEMENTED("setClock");
 * @endcode
 *
 * __builtin_unreachable() tells the compiler that control never reaches the
 * end of the block, suppressing -Wreturn-type warnings.
 */
#define NOT_IMPLEMENTED(msg)                                                   \
  { notImplemented(msg); __builtin_unreachable(); }

/**
 * Log a critical error and throw an Exception.
 *
 * Accepts a printf-style format string.  Always throws — the [[noreturn]]
 * annotation lets the compiler know control will not continue past this call.
 *
 * @param msg printf format string describing the error.
 * @param ... Additional arguments for the format string.
 */
[[noreturn]] void ardulinuxError(const char *msg, ...);

/**
 * Assert that @p result is not negative; throw on failure.
 *
 * Wraps syscall return values that signal errors as negative integers (e.g.
 * POSIX read/write/ioctl).  On failure, prints errno alongside @p msg and
 * throws an Exception.
 *
 * @param result   The integer to check (typically a syscall return value).
 * @param msg      printf format string for the error message.
 * @param ...      Additional arguments for the format string.
 * @return @p result unchanged if it is ≥ 0.
 */
int ardulinuxCheckNotNeg(int result, const char *msg, ...);

/**
 * Assert that @p result is zero; throw on failure.
 *
 * Wraps calls that return 0 on success and a non-zero error code otherwise
 * (e.g. pthread_* functions).
 *
 * @param result   The integer to check (typically a return code).
 * @param msg      printf format string for the error message.
 * @param ...      Additional arguments for the format string.
 * @return @p result unchanged if it is 0.
 */
int ardulinuxCheckZero(int result, const char *msg, ...);

/**
 * Trigger a SIGINT to break into an attached debugger.
 *
 * Has no effect when not running under a debugger; the process will terminate
 * unless the signal is caught by the parent.
 */
void ardulinuxDebug();

/**
 * @defgroup portduino_compat Portduino compatibility aliases
 *
 * Third-party libraries (e.g. the upstream arduino-libraries/WiFi submodule)
 * were originally written against the portduino API which used the
 * "portduino" prefix.  These macros map the old names to the current
 * ArduLinux equivalents so those libraries compile without modification.
 * @{
 */
#define portduinoCheckNotNeg ardulinuxCheckNotNeg ///< @see ardulinuxCheckNotNeg
#define portduinoCheckZero   ardulinuxCheckZero   ///< @see ardulinuxCheckZero
#define portduinoDebug       ardulinuxDebug       ///< @see ardulinuxDebug
/** @} */

#endif // ARDULINUX_UTILITY_H
