// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>

/**
 * Return the XDG data directory for the given application.
 *
 * Reads $XDG_DATA_HOME; falls back to "$HOME/.local/share" if the variable
 * is unset or empty, as required by the XDG Base Directory Specification
 * (https://specifications.freedesktop.org/basedir-spec/latest/).
 *
 * The returned path has the form: <base>/appName  (no trailing slash).
 *
 * @param appName  Application subdirectory name, e.g. "ardulinux".
 * @return         Resolved absolute path string.
 *
 * @pre appName != nullptr
 * @pre $HOME is set in the environment when $XDG_DATA_HOME is unset or empty
 *      (asserted at runtime).
 */
std::string xdgDataDir(const char *appName);
