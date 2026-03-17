// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "AppInfo.h"

// Weak defaults — applications override these by defining the same symbol
// (without __attribute__((weak))) in their own source files.

__attribute__((weak)) const char *ardulinuxAppName = "ardulinux";
__attribute__((weak)) const char *ardulinuxAppDescription = "An application written with ardulinux";

// Use the build-time FIRMWARE_VERSION define when provided (e.g. via
// -DFIRMWARE_VERSION="1.14.1-linux"), otherwise fall back to "unknown".
#ifdef FIRMWARE_VERSION
__attribute__((weak)) const char *ardulinuxAppVersion = FIRMWARE_VERSION;
#else
__attribute__((weak)) const char *ardulinuxAppVersion = "unknown";
#endif
