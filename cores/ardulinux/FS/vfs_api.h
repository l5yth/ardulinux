// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef vfs_api_h
#define vfs_api_h

#include "FS.h"
#include "FSImpl.h"

extern "C" {
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
}

using namespace fs;

class VFSFileImpl;

/**
 * FSImpl implementation that maps the Arduino FS API to a POSIX directory.
 *
 * All paths are relative to the mountpoint set by FSImpl::mountpoint().  The
 * mountpoint is prepended to every path before calling POSIX functions, so
 * application code uses "/data/config.json" and the real path is something
 * like "~/.local/share/ardulinux/default/data/config.json".
 */
class VFSImpl : public FSImpl
{

protected:
    friend class VFSFileImpl; ///< VFSFileImpl needs access to _mountpoint

public:
    /** @copydoc FSImpl::open */
    FileImplPtr open(const char* path, const char* mode = "r") override;
    /** @copydoc FSImpl::exists */
    bool        exists(const char* path) override;
    /** @copydoc FSImpl::rename */
    bool        rename(const char* pathFrom, const char* pathTo) override;
    /** @copydoc FSImpl::remove */
    bool        remove(const char* path) override;
    /** @copydoc FSImpl::mkdir */
    bool        mkdir(const char *path) override;
    /** @copydoc FSImpl::rmdir */
    bool        rmdir(const char *path) override;
};

/**
 * FileImpl backed by a POSIX FILE* (regular file) or DIR* (directory).
 *
 * Constructed by VFSImpl::open().  The mountpoint-prefixed real path is
 * computed in the constructor and used for all subsequent POSIX calls.
 *
 * A VFSFileImpl can be either a file (_f != NULL) or a directory (_d != NULL)
 * but not both; isDirectory() distinguishes between the two.
 */
class VFSFileImpl : public FileImpl
{
protected:
    VFSImpl*            _fs;          ///< Parent file system (for mountpoint)
    FILE *              _f;           ///< stdio file handle (NULL for directories)
    DIR *               _d;           ///< POSIX directory handle (NULL for files)
    char *              _path;        ///< VFS-relative path (as passed to open)
    bool                _isDirectory; ///< True if this handle is a directory
    mutable struct stat _stat;        ///< Cached stat result; refreshed on demand
    mutable bool        _written;     ///< True if data has been written since last stat

    /**
     * Refresh _stat from the filesystem.
     *
     * Called lazily before size() or getLastWrite() to get up-to-date metadata.
     * Clears _written so that the next size() call does not re-stat
     * unnecessarily.
     */
    void _getStat() const;

public:
    /**
     * Construct and open the file or directory at @p path.
     *
     * Prepends the mount point to get the real path, calls stat() to
     * determine whether to open as a file (fopen) or directory (opendir),
     * and creates the file if @p mode allows it and it does not yet exist.
     *
     * @param fs    Parent VFSImpl (for mountpoint lookup).
     * @param path  VFS-relative path starting with '/'.
     * @param mode  fopen-compatible mode string ("r", "w", "a", etc.).
     */
    VFSFileImpl(VFSImpl* fs, const char* path, const char* mode);

    /** Close the file/directory and free _path. */
    ~VFSFileImpl() override;

    /** @copydoc FileImpl::write */
    size_t      write(const uint8_t *buf, size_t size) override;
    /** @copydoc FileImpl::read */
    size_t      read(uint8_t* buf, size_t size) override;
    /** Flush the stdio buffer and fsync to ensure durability. */
    void        flush() override;
    /** @copydoc FileImpl::seek */
    bool        seek(uint32_t pos, SeekMode mode) override;
    /** @copydoc FileImpl::position */
    size_t      position() const override;
    /** @copydoc FileImpl::size */
    size_t      size() const override;
    /** @copydoc FileImpl::close */
    void        close() override;
    /** @copydoc FileImpl::name */
    const char* name() const override;
    /** @copydoc FileImpl::getLastWrite */
    time_t      getLastWrite() override;
    /** @copydoc FileImpl::isDirectory */
    boolean     isDirectory(void) override;
    /**
     * Open the next directory entry.
     *
     * Skips entries that are neither regular files (DT_REG) nor directories
     * (DT_DIR) via tail recursion.
     *
     * @return FileImplPtr to the next entry, or empty ptr at end-of-directory.
     */
    FileImplPtr openNextFile(const char* mode) override;
    /** @copydoc FileImpl::rewindDirectory */
    void        rewindDirectory(void) override;
    /** @copydoc FileImpl::operator bool */
    operator    bool();
};

#endif
