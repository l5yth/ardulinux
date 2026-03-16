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

// This file is intentionally kept free of Arduino/ArduinoCore-API headers.
// ArduinoCore-API's Common.h declares int main() __attribute__((weak))
// unconditionally, which conflicts with int main(int, char**) on GCC 15+
// (main cannot be overloaded). By defining the real entry point here, in a
// translation unit that never sees Common.h, the conflict is avoided without
// patching the upstream submodule.

int ardulinux_main(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    return ardulinux_main(argc, argv);
}
