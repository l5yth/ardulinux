// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for the VFS layer in cores/ardulinux/FS/vfs_api.cpp.
//
// These tests mount a temporary directory as the VFS root and exercise the
// open/read/write/list paths.  A unique tmpdir is created per test fixture
// so tests are isolated and can run in parallel without interference.
//
// Key coverage targets:
//   - VFSImpl::open() success path (regular file and directory)
//   - VFSFileImpl::write() and read() round-trip
//   - VFSFileImpl::openNextFile() drain to end-of-directory (FileImplPtr())
//   - VFSFileImpl::openNextFile() separator insertion logic

#include <catch2/catch_test_macros.hpp>
#include "vfs_api.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

// ─── Helpers ─────────────────────────────────────────────────────────────────

/**
 * Create a unique temporary directory and return its path.
 *
 * The caller is responsible for removing the directory tree when done.
 * Uses mkdtemp() which is POSIX-standardised (no mktemp races).
 *
 * @return Heap-allocated path string; caller must free().
 */
static char *make_tmpdir()
{
    char tmpl[] = "/tmp/ardulinux-vfs-XXXXXX";
    char *path = mkdtemp(tmpl);
    if (!path)
        return nullptr;
    return strdup(path);
}

// ─── VFSImpl::open — basic file operations ───────────────────────────────────

TEST_CASE("VFSImpl::open creates and writes a new file", "[vfs]") {
    char *root = make_tmpdir();
    REQUIRE(root != nullptr);

    VFSImpl vfs;
    vfs.mountpoint(root);

    // Create a new file via 'w' mode.
    auto f = vfs.open("/hello.txt", "w");
    REQUIRE(f);
    const uint8_t data[] = {'H', 'i', '\n'};
    CHECK(f->write(data, sizeof(data)) == sizeof(data));
    f->close();

    // Re-open for reading.
    auto g = vfs.open("/hello.txt", "r");
    REQUIRE(g);
    uint8_t buf[8] = {};
    CHECK(g->read(buf, sizeof(buf)) == sizeof(data));
    CHECK(buf[0] == 'H');
    g->close();

    free(root);
}

TEST_CASE("VFSImpl::open returns invalid FileImplPtr for missing file in read mode", "[vfs]") {
    char *root = make_tmpdir();
    REQUIRE(root != nullptr);

    VFSImpl vfs;
    vfs.mountpoint(root);

    auto f = vfs.open("/no-such-file.txt", "r");
    CHECK(!f);

    free(root);
}

TEST_CASE("VFSImpl::open returns invalid FileImplPtr when mountpoint is NULL", "[vfs]") {
    VFSImpl vfs;
    // _mountpoint is NULL: open() must return an empty ptr without crashing.
    auto f = vfs.open("/anything", "r");
    CHECK(!f);
}

// ─── VFSFileImpl::openNextFile ────────────────────────────────────────────────
//
// This sub-section exercises the lines in openNextFile() that were modified
// in the PR: the end-of-directory return and the separator insertion logic.

TEST_CASE("VFSImpl::open directory returns a valid directory handle", "[vfs]") {
    char *root = make_tmpdir();
    REQUIRE(root != nullptr);

    VFSImpl vfs;
    vfs.mountpoint(root);

    // The root directory itself is accessible at '/'.
    auto dir = vfs.open("/", "r");
    REQUIRE(dir);
    CHECK(dir->isDirectory());
    dir->close();

    free(root);
}

TEST_CASE("VFSFileImpl::openNextFile returns empty ptr at end of directory", "[vfs]") {
    char *root = make_tmpdir();
    REQUIRE(root != nullptr);

    VFSImpl vfs;
    vfs.mountpoint(root);

    // Write two files so the directory has known non-dot entries.
    for (const char *name : {"/a.txt", "/b.txt"}) {
        auto f = vfs.open(name, "w");
        REQUIRE(f);
        const uint8_t byte = 0x42;
        f->write(&byte, 1);
        f->close();
    }

    auto dir = vfs.open("/", "r");
    REQUIRE(dir);
    REQUIRE(dir->isDirectory());

    // Drain all entries (including '.' and '..') until openNextFile() returns
    // an empty ptr (end-of-directory).  readdir on a tmpdir returns '.', '..'
    // plus any created files — all of these are DT_DIR or DT_REG so they are
    // not skipped.  We verify that eventually the end marker is returned.
    FileImplPtr entry;
    int count = 0;
    for (int i = 0; i < 20; i++) {
        entry = dir->openNextFile("r");
        if (!entry) break;
        count++;
    }
    // At least the two files we created must have been seen.
    CHECK(count >= 2);
    // The final call must have returned an empty ptr (end-of-directory).
    CHECK(!entry);

    dir->close();
    free(root);
}

TEST_CASE("VFSFileImpl::openNextFile name includes parent-separator", "[vfs]") {
    char *root = make_tmpdir();
    REQUIRE(root != nullptr);

    VFSImpl vfs;
    vfs.mountpoint(root);

    // Create a subdirectory and a file inside it.
    std::string subdir_path = std::string(root) + "/sub";
    REQUIRE(::mkdir(subdir_path.c_str(), 0700) == 0);
    std::string file_path = subdir_path + "/item.txt";
    FILE *tmp = fopen(file_path.c_str(), "w");
    REQUIRE(tmp != nullptr);
    fputs("data", tmp);
    fclose(tmp);

    auto dir = vfs.open("/sub", "r");
    REQUIRE(dir);
    REQUIRE(dir->isDirectory());

    // openNextFile must produce a path that includes the parent (with '/').
    auto entry = dir->openNextFile("r");
    REQUIRE(entry);
    const char *name = entry->name();
    REQUIRE(name != nullptr);
    // The entry path must contain '/' as a separator between parent and child.
    CHECK(std::string(name).find('/') != std::string::npos);

    dir->close();
    free(root);
}

// ─── VFSFileImpl misc ─────────────────────────────────────────────────────────

TEST_CASE("VFSFileImpl::position and seek work on a regular file", "[vfs]") {
    char *root = make_tmpdir();
    REQUIRE(root != nullptr);

    VFSImpl vfs;
    vfs.mountpoint(root);

    auto f = vfs.open("/pos.txt", "w+");
    REQUIRE(f);
    const uint8_t data[4] = {1, 2, 3, 4};
    f->write(data, sizeof(data));

    CHECK(f->seek(0, SeekSet));
    CHECK(f->position() == 0);
    f->close();

    free(root);
}

TEST_CASE("VFSFileImpl::size returns correct byte count", "[vfs]") {
    char *root = make_tmpdir();
    REQUIRE(root != nullptr);

    VFSImpl vfs;
    vfs.mountpoint(root);

    auto f = vfs.open("/size.txt", "w");
    REQUIRE(f);
    const uint8_t data[7] = {0, 1, 2, 3, 4, 5, 6};
    f->write(data, sizeof(data));
    f->flush();
    f->close();

    auto g = vfs.open("/size.txt", "r");
    REQUIRE(g);
    CHECK(g->size() == 7);
    g->close();

    free(root);
}
