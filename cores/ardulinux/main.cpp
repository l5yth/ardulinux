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

#include "Arduino.h"
#include "AppInfo.h"
#include "ArduLinuxFS.h"
#include "ArduLinuxGPIO.h"
#include "XDGDirs.h"
#include <argp.h>
#include <stdio.h>
#include <ftw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <iostream>
#include <cstring>

/**
 * Milliseconds to sleep at the end of each loop() iteration when no real
 * hardware is bound.  Prevents the process from burning 100% CPU in pure
 * simulation mode.  TODO: make configurable via command-line flag.
 */
static long loopDelay = 100;

/**
 * Copy of argv kept for use by reboot().
 *
 * The --erase/-e flags are stripped so that a reboot does not re-trigger a
 * filesystem wipe.
 */
char **progArgv;

/**
 * Weak hook called before argument parsing.
 *
 * Applications that need to register custom argp child parsers (via
 * ardulinuxAddArguments()) should override this function.  If not overridden,
 * a default message is printed and the default settings are used.
 */
void __attribute__((weak)) ardulinuxSetup() {
  printf("No ardulinuxSetup() found, using default settings...\n");
}

/**
 * Weak hook for very early initialisation, before argument parsing.
 *
 * Override to call ardulinuxAddArguments() and register custom CLI flags.
 * The default implementation is a no-op.
 */
void __attribute__((weak)) ardulinuxCustomInit() {}

// argp descriptor strings — shown in --help output.
// argp_program_bug_address is set at runtime from ardulinuxAppBugAddress in
// ardulinux_main() so applications can override it via the weak symbol.
// doc is set at runtime from ardulinuxAppDescription in ardulinux_main().
// args_doc is empty: the runtime accepts no positional arguments.
static const char args_doc[] = "";

/**
 * argp version-print hook called when @c --version / @c -V is passed.
 *
 * Prints "<appname> <version>\n" to @p stream.  When @c ardulinuxAppName is
 * @c nullptr (not the typical case), falls back to the basename in @c argv[0]
 * via @p state->name so the output is always meaningful.
 *
 * @param stream Output stream (stdout for @c --version).
 * @param state  argp parser state; @c state->name is the argv[0] basename.
 */
static void print_version(FILE *stream, struct argp_state *state) {
    // argp may call this hook with state == NULL in some internal paths,
    // so guard before dereferencing state->name.
    const char *name = ardulinuxAppName ? ardulinuxAppName
                     : (state ? state->name : "ardulinux");
    fprintf(stream, "%s %s\n", name, ardulinuxAppVersion);
}

/** Built-in command-line options recognised by the runtime. */
static struct argp_option options[] = {
    {"erase", 'e', 0, 0, "Erase virtual filesystem before use"},
    {"fsdir", 'd', "DIR", 0, "The directory to use as the virtual filesystem"},
    {0}};

/**
 * Parsed command-line arguments for the ArduLinux runtime.
 *
 * Populated by parse_opt() via argp and consumed by ardulinux_main().
 */
struct TopArguments {
  bool erase;   ///< --erase / -e: wipe the VFS root before mounting
  char *fsDir;  ///< --fsdir / -d DIR: explicit VFS root path (NULL → default)
};

/** Global instance of TopArguments; zero-initialised at program start. */
TopArguments ardulinuxArguments;

/** Space for one optional child argp parser registered by the application. */
static struct argp_child children[2] = {{NULL}, {NULL}};

/** Opaque pointer passed through to the child parser's input field. */
static void *childArguments;

/**
 * argp option parser callback.
 *
 * Called once per recognised option.  Forwards child parser setup on
 * ARGP_KEY_INIT and routes -e / -d to the TopArguments struct.
 *
 * @param key   Option character or ARGP_KEY_* constant.
 * @param arg   Option argument string (NULL for flags without arguments).
 * @param state argp parser state; state->input points to TopArguments.
 * @return 0 on success, ARGP_ERR_UNKNOWN for unrecognised options.
 */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  auto args = (TopArguments *)state->input;
  switch (key) {
  case ARGP_KEY_INIT:
    // Wire the child parser's input pointer before it processes its first key.
    if (children[0].argp)
      state->child_inputs[0] = childArguments;
    break;
  case 'e':
    args->erase = true;
    break;
  case 'd':
    args->fsDir = arg;
    break;
  case ARGP_KEY_ARG:
    return 0;  // Accept (and ignore) positional arguments
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

/**
 * nftw() callback that deletes each file/directory entry below the root.
 *
 * Entries at level 0 (the root directory itself) are deliberately skipped
 * so that the directory remains present after the wipe.
 *
 * Adapted from https://stackoverflow.com/a/5467788
 *
 * @param fpath    Absolute path of the current entry.
 * @param sb       stat structure for the entry (unused).
 * @param typeflag FTW type flag (unused).
 * @param ftwbuf   FTW state; ftwbuf->level is 0 for the root.
 * @return 0 to continue traversal, non-zero to abort.
 */
int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = 0;
    if (0 < ftwbuf->level)  // Skip the top-level directory itself
      rv = remove(fpath);
    if (rv)
        perror(fpath);
    return rv;
}

/**
 * Recursively remove all contents of @p path (but not the directory itself).
 *
 * FTW_DEPTH ensures children are visited before their parent so directories
 * are empty before remove() is called on them.  FTW_PHYS prevents following
 * symbolic links.
 *
 * @param path Root directory whose contents should be removed.
 * @return 0 on success, non-zero on failure.
 */
int rmrf(char *path)
{
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

static struct argp argp = {options, parse_opt, args_doc, nullptr, children, 0, 0};

/**
 * Register an application-specific argp child parser.
 *
 * Call from ardulinuxCustomInit() before argp_parse() runs.  Only one child
 * parser is supported at a time.
 *
 * @param child           Filled argp_child descriptor for the child parser.
 * @param _childArguments Opaque pointer forwarded to the child parser's input.
 */
void ardulinuxAddArguments(const struct argp_child &child,
                           void *_childArguments) {
  children[0] = child;
  childArguments = _childArguments;
}

/**
 * Restart the current process via execv().
 *
 * Replaces the running process image with a fresh instance using the saved
 * progArgv (which has --erase stripped to avoid re-wiping the filesystem).
 * Only returns on execv() failure, in which case it exits with failure status.
 */
void reboot() {
  int err = execv(progArgv[0], progArgv);
  printf("execv() returned %i!\n", err);
  std::cout << "error: " << std::strerror(errno) << '\n';
  exit(EXIT_FAILURE);
}

/**
 * ArduLinux runtime entry point.
 *
 * Performs the full startup sequence:
 *  1. Build a --erase-stripped copy of argv for use by reboot().
 *  2. Call ardulinuxCustomInit() so the application can register CLI flags.
 *  3. Parse command-line arguments.
 *  4. Mount the virtual filesystem (default: $XDG_DATA_HOME/ardulinux/default/).
 *  5. Call gpioInit() to populate the pin table with SimGPIOPin instances.
 *  6. Call ardulinuxSetup() so the application can bind real hardware pins.
 *  7. Call Arduino setup().
 *  8. Run the main loop: gpioIdle() → loop() → optional 100 ms sleep.
 *
 * The loop sleep is skipped when realHardware is true (i.e., at least one
 * LinuxGPIOPin has been registered) to allow ISR polling at full speed.
 *
 * @param argc Argument count from main().
 * @param argv Argument vector from main().
 * @return 0 on normal exit, or argp error code on argument parse failure.
 */
int ardulinux_main(int argc, char *argv[]) {

  // Build a new argv with --erase/-e removed so that reboot() does not
  // re-trigger a filesystem wipe on restart.
  progArgv = (char**) malloc((argc + 1) * sizeof(char*)); // +1 for NULL terminator
  int j = 0;
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-e") != 0 && strcmp(argv[i], "--erase") != 0) {
      progArgv[j] = argv[i];
      j++;
    }
  }
  progArgv[j] = NULL;  // NULL-terminate the new argv

  ardulinuxCustomInit();

  // Apply app-provided metadata before argp parses.
  argp.doc = ardulinuxAppDescription;
  argp_program_version_hook = print_version;
  argp_program_bug_address = ardulinuxAppBugAddress;

  auto *args = &ardulinuxArguments;

  auto parseResult = argp_parse(&argp, argc, argv, 0, 0, args);
  if (parseResult == 0) {
    String fsRoot;

    if (!args->fsDir) {
      // Construct default VFS root: $XDG_DATA_HOME/ardulinux/default/
      fsRoot = xdgDataDir(ardulinuxAppName).c_str();
      mkdir(fsRoot.c_str(), 0700);  // Create $XDG_DATA_HOME/ardulinux if needed (ignore EEXIST)
      fsRoot += "/default";
    } else
      fsRoot += args->fsDir;

    printf("%s is starting, VFS root at %s\n", ardulinuxAppName, fsRoot.c_str());

    int status = mkdir(fsRoot.c_str(), 0700);
    if (status != 0 && errno == EEXIST && args->erase) {
      // Directory already exists and --erase was requested: wipe its contents.
      std::cout << "Erasing virtual Filesystem!" << std::endl;
      rmrf(const_cast<char*>(fsRoot.c_str()));
    }

    ardulinuxVFS->mountpoint(fsRoot.c_str());

    gpioInit();
    ardulinuxSetup();
    setup();

    while (true) {
      gpioIdle();  // Poll GPIO ISRs before each loop() invocation
      loop();

      // When no real hardware is present, sleep to avoid burning CPU on
      // pure-simulation workloads.  Real-hardware builds skip this so ISR
      // polling keeps up with hardware event rates.
      if (!realHardware)
        delay(loopDelay);
    }
    return 0;
  } else
    return parseResult;
}
