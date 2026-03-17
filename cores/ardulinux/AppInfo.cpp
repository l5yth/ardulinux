// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "AppInfo.h"

// Weak defaults — applications override these by defining the same symbol
// (without __attribute__((weak))) in their own source files.

__attribute__((weak)) const char *ardulinuxAppName = "ardulinux";
__attribute__((weak)) const char *ardulinuxAppDescription = "An application written with ardulinux";

// Use the build-time FIRMWARE_VERSION define when provided, otherwise fall
// back to "unknown".  FIRMWARE_VERSION must be a string literal — pass it
// with escaped quotes in CMake:
//
//   target_compile_definitions(myapp PRIVATE "FIRMWARE_VERSION=\"1.2.3\"")
//
// which generates -DFIRMWARE_VERSION="1.2.3" on the compiler command line.
// Passing it without quotes (e.g. -DFIRMWARE_VERSION=1.2.3) is a compile
// error because the bare token sequence is not a valid initialiser for a
// const char *.  Stringification macros are not used here because they would
// double-escape an already-quoted string literal, producing the wrong value.
#ifdef FIRMWARE_VERSION
__attribute__((weak)) const char *ardulinuxAppVersion = FIRMWARE_VERSION;
#else
__attribute__((weak)) const char *ardulinuxAppVersion = "unknown";
#endif
