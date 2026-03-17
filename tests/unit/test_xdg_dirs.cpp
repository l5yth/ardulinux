// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for XDGDirs.cpp (xdgDataDir).
//
// All tests manipulate environment variables and restore them afterwards.
// No filesystem I/O is performed.

#include <catch2/catch_test_macros.hpp>
#include "XDGDirs.h"

#include <cstdlib>
#include <string>

// ─── Helper ───────────────────────────────────────────────────────────────────

/**
 * RAII guard that saves an environment variable on construction and restores
 * it (or unsets it) on destruction.
 */
struct EnvGuard {
    std::string name;
    std::string saved;
    bool was_set;

    explicit EnvGuard(const char *varName) : name(varName) {
        const char *v = getenv(varName);
        was_set = (v != nullptr);
        if (was_set) saved = v;
    }

    ~EnvGuard() {
        if (was_set)
            setenv(name.c_str(), saved.c_str(), 1);
        else
            unsetenv(name.c_str());
    }
};

// ─── xdgDataDir ───────────────────────────────────────────────────────────────

TEST_CASE("xdgDataDir uses XDG_DATA_HOME when set", "[xdg]") {
    EnvGuard guard("XDG_DATA_HOME");
    setenv("XDG_DATA_HOME", "/custom/data", 1);

    std::string result = xdgDataDir("myapp");

    CHECK(result == "/custom/data/myapp");
}

TEST_CASE("xdgDataDir appName appears as subdirectory under XDG_DATA_HOME", "[xdg]") {
    EnvGuard guard("XDG_DATA_HOME");
    setenv("XDG_DATA_HOME", "/some/base", 1);

    std::string result = xdgDataDir("myapp");

    // Must end with /myapp — not embedded elsewhere in the path.
    CHECK(result.rfind("/myapp") == result.size() - 6);
}

TEST_CASE("xdgDataDir falls back to HOME/.local/share when XDG_DATA_HOME is unset", "[xdg]") {
    EnvGuard xdgGuard("XDG_DATA_HOME");
    EnvGuard homeGuard("HOME");
    unsetenv("XDG_DATA_HOME");
    setenv("HOME", "/home/testuser", 1);

    std::string result = xdgDataDir("myapp");

    CHECK(result == "/home/testuser/.local/share/myapp");
}

TEST_CASE("xdgDataDir falls back to HOME/.local/share when XDG_DATA_HOME is empty", "[xdg]") {
    EnvGuard xdgGuard("XDG_DATA_HOME");
    EnvGuard homeGuard("HOME");
    setenv("XDG_DATA_HOME", "", 1);   // empty string — treated as unset per XDG spec
    setenv("HOME", "/home/testuser", 1);

    std::string result = xdgDataDir("myapp");

    CHECK(result == "/home/testuser/.local/share/myapp");
}

TEST_CASE("xdgDataDir appName appears as subdirectory under HOME fallback", "[xdg]") {
    EnvGuard xdgGuard("XDG_DATA_HOME");
    EnvGuard homeGuard("HOME");
    unsetenv("XDG_DATA_HOME");
    setenv("HOME", "/home/testuser", 1);

    std::string result = xdgDataDir("ardulinux");

    CHECK(result.rfind("/ardulinux") == result.size() - 10);
}

TEST_CASE("xdgDataDir result contains no trailing slash", "[xdg]") {
    EnvGuard guard("XDG_DATA_HOME");
    setenv("XDG_DATA_HOME", "/base", 1);

    std::string result = xdgDataDir("myapp");

    CHECK(result.back() != '/');
}
