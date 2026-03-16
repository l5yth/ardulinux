from SCons.Script import AlwaysBuild, Default, DefaultEnvironment

env = DefaultEnvironment()

target_bin = env.BuildProgram()

Default(target_bin)
AlwaysBuild(env.Alias("nobuild", target_bin))
env.Alias("buildprog", target_bin, target_bin)
