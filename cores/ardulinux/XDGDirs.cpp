// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "XDGDirs.h"

#include <cassert>
#include <cstdlib>

std::string xdgDataDir(const char *appName)
{
    assert(appName);

    const char *base = getenv("XDG_DATA_HOME");
    if (base && base[0] != '\0')
        return std::string(base) + "/" + appName;

    // XDG spec §3: if $XDG_DATA_HOME is unset or empty, default to $HOME/.local/share
    const char *home = getenv("HOME");
    assert(home);
    return std::string(home) + "/.local/share/" + appName;
}
