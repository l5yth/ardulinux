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

#include <itoa.h>
#include <string>
#include <stdexcept>
#include <stdio.h>


/**
 * Map a numeric radix to the corresponding printf format string.
 *
 * Only octal (8), decimal (10), and hexadecimal (16) are supported.
 *
 * @param radix  Numeric base: 8, 10, or 16.
 * @return printf format string ("%o", "%d", or "%X").
 * @throws std::runtime_error for unsupported radix values.
 */
std::string radixToFmtString(int const radix)
{
    if (radix == 8)       return std::string("%o");
    else if (radix == 10) return std::string("%d");
    else if (radix == 16) return std::string("%X");
    else throw std::runtime_error("Invalid radix.");
}

/**
 * Convert a signed integer to a string in the given radix.
 *
 * Arduino compatibility shim; the standard library itoa() is not available
 * on all platforms.
 *
 * @param value  Value to convert.
 * @param str    Output buffer (caller must ensure sufficient space).
 * @param radix  Numeric base: 8, 10, or 16.
 * @return @p str.
 */
char * itoa(int value, char * str, int radix)
{
    sprintf(str, radixToFmtString(radix).c_str(), value);
    return str;
}

/**
 * Convert a signed long to a string in the given radix.
 *
 * @param value  Value to convert.
 * @param str    Output buffer.
 * @param radix  Numeric base: 8, 10, or 16.
 * @return @p str.
 */
char * ltoa(long value, char * str, int radix)
{
    sprintf(str, radixToFmtString(radix).c_str(), value);
    return str;
}

/**
 * Convert an unsigned integer to a string in the given radix.
 *
 * @param value  Value to convert.
 * @param str    Output buffer.
 * @param radix  Numeric base: 8, 10, or 16.
 * @return @p str.
 */
char * utoa(unsigned value, char *str, int radix)
{
    sprintf(str, radixToFmtString(radix).c_str(), value);
    return str;
}

/**
 * Convert an unsigned long to a string in the given radix.
 *
 * @param value  Value to convert.
 * @param str    Output buffer.
 * @param radix  Numeric base: 8, 10, or 16.
 * @return @p str.
 */
char * ultoa(unsigned long value, char * str, int radix)
{
    sprintf(str, radixToFmtString(radix).c_str(), value);
    return str;
}
