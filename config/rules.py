import argparse
import hancho

# FIXME add build_type to build directory

#-------------------------------------------------------------------------------

parser = argparse.ArgumentParser()

parser.add_argument('--verbose',  default=False, action='store_true', help='Print verbose build info')
parser.add_argument('--serial',   default=False, action='store_true', help='Do not parallelize commands')
parser.add_argument('--dryrun',   default=False, action='store_true', help='Do not run commands')
parser.add_argument('--debug',    default=False, action='store_true', help='Dump debugging information')
parser.add_argument('--release',  default=False, action='store_true', help='Release-mode optimized build')
parser.add_argument('--force',    default=False, action='store_true', help='Force rebuild of everything')
parser.add_argument('--quiet',    default=False, action='store_true', help='Mute command output')
(flags, unrecognized) = parser.parse_known_args()

hancho.config.verbose = flags.verbose
hancho.config.serial  = flags.serial
hancho.config.dryrun  = flags.dryrun
hancho.config.debug   = flags.debug
hancho.config.force   = flags.force
hancho.config.quiet   = flags.quiet

#-------------------------------------------------------------------------------

base_rule = hancho.base_rule.extend(
  build_type  = "release" if flags.release else "debug",
  toolchain   = "x86_64-linux-gnu",
)

#-------------------------------------------------------------------------------

compile_cpp = base_rule.extend(
  description = "Compiling {files_in[0]} -> {files_out[0]} ({build_type})",
  command     = "{toolchain}-g++ {cpp_std} {gcc_opt} {warnings} {includes} {defines} -c {files_in[0]} -o {files_out[0]}",

  cpp_std     = "-std=c++20",
  gcc_opt     = "{'-O3' if build_type == 'release' else '-g -O0'} -MMD",
  warnings    = "-Wall -Werror -Wno-unused-variable -Wno-unused-local-typedefs -Wno-unused-but-set-variable",
  includes    = "-I.",
  files_out   = "{swap_ext(files_in[0], '.o')}",
  depfile     = "{swap_ext(files_in[0], '.d')}",
)

link_c_lib = base_rule.extend(
  description = "Bundling {files_out[0]}",
  command     = "ar rcs {files_out[0]} {join(files_in)}",
)

link_c_bin = base_rule.extend(
  description = "Linking {files_out[0]}",
  command     = "{toolchain}-g++ {ld_opt} {warnings} {join(files_in)} {join(deps)} {sys_libs} -o {files_out[0]}",

  ld_opt      = "{'-O3' if build_type == 'release' else '-g -O0'}",
  warnings    = "-Wall",
)

test_rule = base_rule.extend(
  description = "Running test {files_in[0]}",
  command     = "rm -f {files_out[0]} && {files_in[0]} {args} && touch {files_out[0]}",
  files_out   = "{files_in[0]}_pass",
)

#-------------------------------------------------------------------------------

def compile_srcs(srcs, **kwargs):
  return [compile_cpp(files_in = f, **kwargs) for f in hancho.flatten(srcs)]

def c_binary(*, name, srcs, **kwargs):
  return link_c_bin(
    files_in  = compile_srcs(srcs, **kwargs),
    files_out = name,
    **kwargs)

def c_library(*, name, srcs, **kwargs):
  return link_c_lib(
    files_in  = compile_srcs(srcs, **kwargs),
    files_out = name,
    **kwargs)

def c_test(*, name, srcs, **kwargs):
  test_bin = c_binary(name = name, srcs = srcs, **kwargs)
  test_rule(files_in = test_bin, **kwargs)

#-------------------------------------------------------------------------------
