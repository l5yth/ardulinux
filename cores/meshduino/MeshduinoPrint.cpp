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

#include "Utility.h"
#include <Print.h>

#include <cstdarg>

#define MAX_STR_LEN 256

size_t Print::printf(const char *format, ...) {
    char buf[MAX_STR_LEN]; // FIXME, this burns a lot of stack, but on
                                // Linux that is fine, TBD on MyNewt
    va_list args;
    va_start(args, format);
    size_t n = vsnprintf(buf, sizeof(buf), format, args);
    write(buf);
    va_end(args);
    return n;
}