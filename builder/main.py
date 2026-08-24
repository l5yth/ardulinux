import os
from os.path import basename, join
from SCons.Script import AlwaysBuild, Default, DefaultEnvironment

env = DefaultEnvironment()

# Honor a caller-supplied cross toolchain from the environment so ArduLinux can
# be built by external build systems (OpenWRT/buildroot, Yocto, Debian rules)
# that provide their own compiler and sysroot.  This runs before the framework
# script (main.py -> BuildProgram -> ProcessProgramDeps -> BuildFrameworks ->
# arduino.py), so the override reaches the core/framework objects as well as the
# application's.  Each variable is optional; when unset, PlatformIO's native
# gcc/g++ defaults are kept, so ordinary host builds are unaffected.  Compiler
# *flags* are intentionally left to the caller (e.g. via PLATFORMIO_BUILD_FLAGS)
# rather than parsed here.
for _scons_var, _env_var in (
    ("CC", "TARGET_CC"),
    ("CXX", "TARGET_CXX"),
    ("AR", "TARGET_AR"),
    ("RANLIB", "TARGET_RANLIB"),
):
    _tool = os.environ.get(_env_var)
    if _tool:
        env.Replace(**{_scons_var: _tool})

# Allow the application to override the output binary name.
# Set board_build.progname in platformio.ini to change the default "program".
progname = env.BoardConfig().get("build.progname", "")
if progname:
    # Strip any directory components so a value like "../evil" cannot place
    # the binary outside $BUILD_DIR.
    progname = basename(progname)
    env.Replace(
        PROGNAME=progname,
        PROGPATH=join(env.subst("$BUILD_DIR"), progname),
    )

target_bin = env.BuildProgram()

Default(target_bin)
AlwaysBuild(env.Alias("nobuild", target_bin))
env.Alias("buildprog", target_bin, target_bin)
