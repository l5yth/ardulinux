// ArduLinux - Arduino API for Linux
// Copyright (c) 2011-19 Arduino LLC.
// Copyright (c) 2020-23 Geeksville Industries, LLC
// Copyright (c) 2024-26 jp-bennett
// Copyright (c) 2026-27 l5yth
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#pragma once

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "Utility.h"

/**
 * RAII wrapper around a POSIX file descriptor.
 *
 * Opens a file on construction and closes it on destruction.  All ioctl()
 * calls are routed through the member function which throws ardulinuxError
 * on failure, eliminating per-call error checking in subclasses.
 *
 * Intended to be used as a private base class (e.g. LinuxSPIChip inherits
 * privately from PosixFile) so that file-descriptor management is
 * encapsulated and not accessible from the outside.
 */
class PosixFile
{
    int fd; ///< Underlying file descriptor; -1 after close

public:
    /**
     * Open a file by path.
     *
     * @param path  File system path to open.
     * @param mode  Flags passed to open(2) (default 0 = O_RDONLY).
     * @throws Exception (via ardulinuxError) if open() fails.
     */
    PosixFile(const char *path, int mode = 0)
    {
        fd = open(path, mode);
        if (fd == -1)
            ardulinuxError("Failed to open posix file %s, errno=%d", path, errno);
    }

    /**
     * Adopt an already-open file descriptor.
     *
     * @param _fd  An open file descriptor that this object takes ownership of.
     */
    PosixFile(int _fd) : fd(_fd) {}

    /** Close the file descriptor if still open. */
    ~PosixFile()
    {
        if (fd >= 0)
            close(fd);
        fd = -1;
    }

    /**
     * Issue a device-control ioctl on the managed file descriptor.
     *
     * @param op   ioctl request code (e.g. SPI_IOC_WR_MODE).
     * @param arg  Pointer to the request-specific argument structure.
     * @return The ioctl return value on success.
     * @throws Exception (via ardulinuxError) if the file descriptor is
     *         invalid or the ioctl returns -1.
     */
    int ioctl(unsigned long op, void *arg)
    {
        if (fd < 0)
            ardulinuxError("no file descriptor, errno=%d", errno);

        auto ret = ::ioctl(fd, op, arg);
        if (ret == -1)
            ardulinuxError("ioctl failed, errno=%d", errno);
        return ret;
    }
};
