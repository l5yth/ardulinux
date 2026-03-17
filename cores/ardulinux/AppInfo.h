// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

/**
 * Application name used in startup messages, XDG data paths, and
 * system-level identifiers (e.g. the libgpiod consumer label).
 *
 * The platform provides a weak default of @c "ardulinux".  Override it by
 * defining the symbol in any application source file — no header needed:
 *
 * @code
 * const char *ardulinuxAppName = "meshcored";
 * @endcode
 *
 * The override must be a plain (non-weak) definition at file scope so the
 * linker selects it over the platform default.
 */
extern const char *ardulinuxAppName;

/**
 * One-line description of the application shown in @c --help output.
 *
 * Override the weak default in your application source:
 *
 * @code
 * const char *ardulinuxAppDescription = "Radio mesh daemon";
 * @endcode
 */
extern const char *ardulinuxAppDescription;

/**
 * Version string printed by @c --version, in the format expected by argp:
 * @c "<appname> <version>".
 *
 * The platform default is taken from the @c FIRMWARE_VERSION preprocessor
 * define if it is set at compile time, otherwise it falls back to @c "unknown".
 * @c FIRMWARE_VERSION must be a string literal — pass it with escaped inner
 * quotes in CMake:
 * @code
 * target_compile_definitions(myapp PRIVATE "FIRMWARE_VERSION=\"1.2.3\"")
 * @endcode
 *
 * Override the weak default in your application source:
 *
 * @code
 * const char *ardulinuxAppVersion = "1.14.1-linux";
 * @endcode
 *
 * @note @c nullptr is not a valid value; argp will segfault if the version
 *       hook dereferences a null pointer.
 */
extern const char *ardulinuxAppVersion;
