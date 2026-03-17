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
 * @pre The XDG base directory ($XDG_DATA_HOME or $HOME/.local/share) already
 *      exists on the filesystem.  This function only returns a path string; it
 *      does not create any directories.  Callers that need the returned path to
 *      be writable must ensure the parent exists (e.g. via mkdir) before use.
 * @note If $XDG_DATA_HOME contains a trailing slash the returned path will
 *       contain a double slash (e.g. "/custom//appName").  This is valid per
 *       POSIX but callers that require a canonical path should normalise it.
 */
std::string xdgDataDir(const char *appName);
