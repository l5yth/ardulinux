/*
 FS.h - file system wrapper
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

#ifndef FS_H
#define FS_H

#include <memory>
#include <Arduino.h>

namespace fs
{

/** fopen mode string for read-only access. */
#define FILE_READ       "r"
/** fopen mode string for write (create/truncate) access. */
#define FILE_WRITE      "w"
/** fopen mode string for append access. */
#define FILE_APPEND     "a"

class File;

class FileImpl;
typedef std::shared_ptr<FileImpl> FileImplPtr;
class FSImpl;
typedef std::shared_ptr<FSImpl> FSImplPtr;

/**
 * Seek mode for File::seek().
 *
 * Values map directly to the POSIX fseek() whence constants.
 */
enum SeekMode {
    SeekSet = 0, ///< Seek to absolute position from start of file
    SeekCur = 1, ///< Seek relative to current position
    SeekEnd = 2  ///< Seek relative to end of file
};

/**
 * A reference to an open file or directory.
 *
 * File is a thin wrapper around a shared FileImplPtr so that it can be
 * returned by value from FS::open() and passed around without manual lifetime
 * management.  An empty File (default-constructed or after close()) evaluates
 * to false.
 */
class File : public Stream
{
public:
    /** Construct a File wrapping the given impl pointer (empty if NULL). */
    File(FileImplPtr p = FileImplPtr()) : _p(p) {
        _timeout = 0;
    }

    /** Write a single byte; returns 1 on success. */
    size_t write(uint8_t) override;
    /** Write @p size bytes from @p buf; returns bytes written. */
    size_t write(const uint8_t *buf, size_t size) override;
    /** Return bytes available to read without blocking. */
    int available() override;
    /** Read a single byte; returns -1 at EOF. */
    int read() override;
    /** Peek at the next byte without consuming it; returns -1 if not supported. */
    int peek() override;
    /** Flush pending writes. */
    void flush() override;
    /** Read up to @p size bytes into @p buf; returns bytes read. */
    size_t read(uint8_t* buf, size_t size);
    /** Convenience wrapper: read chars into a char buffer. */
    size_t readBytes(char *buffer, size_t length)
    {
        return read((uint8_t*)buffer, length);
    }

    /** Seek to @p pos with the given @p mode. */
    bool seek(uint32_t pos, SeekMode mode);
    /** Seek to @p pos from the start of the file. */
    bool seek(uint32_t pos)
    {
        return seek(pos, SeekSet);
    }
    /** Return the current read/write position. */
    size_t position() const;
    /** Return the file size in bytes. */
    size_t size() const;
    /** Close the file and release the impl. */
    void close();
    /** Return true if the file is open and valid. */
    operator bool() const;
    /** Return the last-modified timestamp (Unix epoch). */
    time_t getLastWrite();
    /** Return the file path as passed to open(). */
    const char* name() const;

    /** Return true if this is a directory handle. */
    boolean isDirectory(void);
    /** Open the next entry in a directory. */
    File openNextFile(const char* mode = FILE_READ);
    /** Reset the directory iterator. */
    void rewindDirectory(void);

protected:
    FileImplPtr _p; ///< Shared implementation pointer
};

/**
 * File system abstraction providing the standard Arduino FS API.
 *
 * FS delegates all operations to an FSImplPtr (typically a VFSImpl instance).
 * Paths must start with '/'.
 */
class FS
{
public:
    /** Construct with the given impl (e.g. VFSImpl). */
    FS(FSImplPtr impl) : _impl(impl) { }

    /** Open @p path with @p mode (FILE_READ, FILE_WRITE, or FILE_APPEND). */
    File open(const char* path, const char* mode = FILE_READ);
    /** @overload */
    File open(const String& path, const char* mode = FILE_READ);

    /** Return true if @p path exists. */
    bool exists(const char* path);
    /** @overload */
    bool exists(const String& path);

    /** Delete the file at @p path. */
    bool remove(const char* path);
    /** @overload */
    bool remove(const String& path);

    /** Rename @p pathFrom to @p pathTo. */
    bool rename(const char* pathFrom, const char* pathTo);
    /** @overload */
    bool rename(const String& pathFrom, const String& pathTo);

    /** Create directory @p path. */
    bool mkdir(const char *path);
    /** @overload */
    bool mkdir(const String &path);

    /** Remove directory @p path. */
    bool rmdir(const char *path);
    /** @overload */
    bool rmdir(const String &path);


protected:
    FSImplPtr _impl; ///< Concrete file system implementation
};

} // namespace fs

#ifndef FS_NO_GLOBALS
using fs::FS;
using fs::File;
using fs::SeekMode;
using fs::SeekSet;
using fs::SeekCur;
using fs::SeekEnd;
#endif //FS_NO_GLOBALS

#endif //FS_H
