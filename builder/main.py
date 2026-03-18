from os.path import basename, join
from SCons.Script import AlwaysBuild, Default, DefaultEnvironment

env = DefaultEnvironment()

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
