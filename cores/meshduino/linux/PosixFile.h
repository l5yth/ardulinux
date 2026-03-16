// Meshduino - Arduino API for Linux
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

/** A posix file descriptor based file
 */


class PosixFile
{
    int fd;

public:
    PosixFile(const char *path, int mode = 0)
    {
        fd = open(path, mode);
        if (fd == -1)
            meshduinoError("Failed to open posix file %s, errno=%d", path, errno); // FIXME, find a more structured/standard way to throw C++ OS exceptions
    }

    PosixFile(int _fd) : fd(_fd) {}

    ~PosixFile()
    {
        if (fd >= 0)
            close(fd);
        fd = -1;
    }

    int ioctl(unsigned long op, void *arg)
    {
        if(fd < 0)
            meshduinoError("no file descriptor, errno=%d", errno);

        auto ret = ::ioctl(fd, op, arg);
        if (ret == -1)
            meshduinoError("ioctl failed, errno=%d", errno);
        return ret;
    }
};