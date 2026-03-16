/*
 FSImpl.h - base file system interface
 Copyright (c) 2015 Ivan Grokhotkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef FSIMPL_H
#define FSIMPL_H

#include <stddef.h>
#include <stdint.h>

namespace fs
{

/**
 * Abstract interface for a single open file (or directory).
 *
 * VFSFileImpl is the concrete implementation used by ArduLinux.  This
 * interface mirrors the Arduino FS File abstraction so that application code
 * is portable across different storage backends.
 */
class FileImpl
{
public:
    virtual ~FileImpl() { }
    /** Write @p size bytes from @p buf; return bytes written. */
    virtual size_t write(const uint8_t *buf, size_t size) = 0;
    /** Read up to @p size bytes into @p buf; return bytes read. */
    virtual size_t read(uint8_t* buf, size_t size) = 0;
    /** Flush any pending writes to the underlying storage. */
    virtual void flush() = 0;
    /** Seek to @p pos using the given @p mode (SeekSet/SeekCur/SeekEnd). */
    virtual bool seek(uint32_t pos, SeekMode mode) = 0;
    /** Return the current read/write position. */
    virtual size_t position() const = 0;
    /** Return the total file size in bytes. */
    virtual size_t size() const = 0;
    /** Close the file and release associated resources. */
    virtual void close() = 0;
    /** Return the last-modified timestamp (Unix epoch seconds). */
    virtual time_t getLastWrite() = 0;
    /** Return the file path as originally opened. */
    virtual const char* name() const = 0;
    /** Return true if this handle refers to a directory. */
    virtual boolean isDirectory(void) = 0;
    /** Open the next entry inside this directory. */
    virtual FileImplPtr openNextFile(const char* mode) = 0;
    /** Reset the directory iterator to the first entry. */
    virtual void rewindDirectory(void) = 0;
    /** Return true if the file/directory is open and valid. */
    virtual operator bool() = 0;
};

/**
 * Abstract interface for a file system.
 *
 * Stores the VFS mount point and provides the factory methods used by
 * FS::open(), FS::exists(), etc.  VFSImpl is the concrete implementation.
 */
class FSImpl
{
protected:
    const char * _mountpoint; ///< Absolute path used as the root of the VFS

public:
    FSImpl() : _mountpoint(NULL) { }
    virtual ~FSImpl() { }

    /** Open @p path with mode @p mode (e.g. "r", "w", "a"). */
    virtual FileImplPtr open(const char* path, const char* mode) = 0;
    /** Return true if @p path exists in the file system. */
    virtual bool exists(const char* path) = 0;
    /** Rename @p pathFrom to @p pathTo; return true on success. */
    virtual bool rename(const char* pathFrom, const char* pathTo) = 0;
    /** Delete the file at @p path; return true on success. */
    virtual bool remove(const char* path) = 0;
    /** Create directory @p path; return true on success. */
    virtual bool mkdir(const char *path) = 0;
    /** Remove directory @p path; return true on success. */
    virtual bool rmdir(const char *path) = 0;

    /** Set the VFS root directory (must be called before any open()). */
    void mountpoint(const char *);
    /** Return the currently configured VFS root directory. */
    const char * mountpoint();
};

} // namespace fs

#endif //FSIMPL_H
