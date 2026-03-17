// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "AppInfo.h"

// Weak defaults — applications override these by defining the same symbol
// (without __attribute__((weak))) in their own source files.

__attribute__((weak)) const char *ardulinuxAppName = "ardulinux";
__attribute__((weak)) const char *ardulinuxAppDescription = "An application written with ardulinux";
