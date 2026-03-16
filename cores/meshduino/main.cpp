#include "Arduino.h"
#include "MeshduinoFS.h"
#include "MeshduinoGPIO.h"
#include <argp.h>
#include <stdio.h>
#include <ftw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <iostream>
#include <cstring>

/** # msecs to sleep each loop invocation.  FIXME - make this controlable via
 * config file or command line flags.
 */
static long loopDelay = 100;

/** Store pointer to argv for restart
*/
char **progArgv;

/** apps run under meshduino can optionally define a meshduinoSetup() to
 * use meshduino specific init code (such as gpioBind) to setup meshduino on
 * their host machine, before running 'arduino' code.
 */
void __attribute__((weak)) meshduinoSetup() {
  printf("No meshduinoSetup() found, using default settings...\n");
}

void __attribute__((weak)) meshduinoCustomInit() {}

// FIXME - move into app client (out of lib) and use real name
// FIXME - add app specific options as child options
// http://www.gnu.org/software/libc/manual/html_node/Argp.html
const char *argp_program_bug_address =
    "https://github.com/meshcore-dev/Meshcore-device";
static char doc[] = "An application written with meshduino";
static char args_doc[] = "...";

static struct argp_option options[] = {
    {"erase", 'e', 0, 0, "Erase virtual filesystem before use"},
    {"fsdir", 'd', "DIR", 0, "The directory to use as the virtual filesystem"},
    {0}};

struct TopArguments {
  bool erase;
  char *fsDir;
};

// In bss (inited to zero)
TopArguments meshduinoArguments;

static struct argp_child children[2] = {{NULL}, {NULL}};

static void *childArguments;

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  auto args = (TopArguments *)state->input;
  switch (key) {
  case ARGP_KEY_INIT:
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
    return 0;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

/*
 * Functions to remove contents of directory
 * Adapted from: https://stackoverflow.com/a/5467788 
 */ 
int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = 0; 
    if (0 < ftwbuf->level)
      rv = remove(fpath);
    if (rv)
        perror(fpath);

    return rv;
}

int rmrf(char *path)
{
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

static struct argp argp = {options, parse_opt, args_doc, doc, children, 0, 0};

/**
 * call from portuinoCustomInit() if you want to add custom command line
 * arguments
 */
void meshduinoAddArguments(const struct argp_child &child,
                           void *_childArguments) {
  // We only support one child for now
  children[0] = child;
  childArguments = _childArguments;
}

void reboot() {
  int err = execv(progArgv[0], progArgv);
  printf("execv() returned %i!\n", err);
  std::cout << "error: " << std::strerror(errno) << '\n';
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

  progArgv = (char**) malloc((argc + 1) * sizeof(char*)); // New pointer array, argc + 1 to hold the final null
  int j = 0;
  for (int i = 0; i < argc; i++) { // iterate through the arguments, stripping out the erase command, to avoid erase on reboot()
    if (strcmp(argv[i], "-e") != 0 && strcmp(argv[i], "--erase") != 0  ) {
      progArgv[j] = argv[i];
      j++;
    }
  }
  progArgv[j] = NULL;

  meshduinoCustomInit();

  auto *args = &meshduinoArguments;

  auto parseResult = argp_parse(&argp, argc, argv, 0, 0, args);
  if (parseResult == 0) {
    String fsRoot;

    if (!args->fsDir) {
      // create a default dir

      const char *homeDir = getenv("HOME");
      assert(homeDir);

      fsRoot += homeDir + String("/.meshduino");
      mkdir(fsRoot.c_str(), 0700);

      const char *instanceName = "default";
      fsRoot += "/" + String(instanceName);
    } else
      fsRoot += args->fsDir;

    printf("Meshduino is starting, VFS root at %s\n", fsRoot.c_str());

    int status = mkdir(fsRoot.c_str(), 0700);
    if (status != 0 && errno == EEXIST && args->erase) {
      // Remove contents of existing VFS root directory
      std::cout << "Erasing virtual Filesystem!" << std::endl;
      rmrf(const_cast<char*>(fsRoot.c_str())); 
    }

    meshduinoVFS->mountpoint(fsRoot.c_str());

    gpioInit();
    meshduinoSetup();
    setup();

    while (true) {
      gpioIdle(); // FIXME, do this someplace better
      loop();

      // Even if the Arduino code doesn't want to sleep, ensure we don't  burn
      // too much CPU
      if (!realHardware)
        delay(loopDelay);
    }
    return 0;
  } else
    return parseResult;
}
