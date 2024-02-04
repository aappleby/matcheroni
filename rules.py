import argparse
import hancho

#-------------------------------------------------------------------------------

parser = argparse.ArgumentParser()

parser.add_argument('--verbose',  default=False, action='store_true', help='Print verbose build info')
parser.add_argument('--serial',   default=False, action='store_true', help='Do not parallelize commands')
parser.add_argument('--dryrun',   default=False, action='store_true', help='Do not run commands')
parser.add_argument('--debug',    default=False, action='store_true', help='Dump debugging information')
parser.add_argument('--dotty',    default=False, action='store_true', help='Dump dependency graph as dotty')
parser.add_argument('--release',  default=False, action='store_true', help='Release-mode optimized build')
parser.add_argument('--force',    default=False, action='store_true', help='Force rebuild of everything')
(flags, unrecognized) = parser.parse_known_args()

hancho.config.verbose = flags.verbose
hancho.config.serial  = flags.serial
hancho.config.dryrun  = flags.dryrun
hancho.config.debug   = flags.debug
hancho.config.dotty   = flags.dotty
hancho.config.force   = flags.force

#-------------------------------------------------------------------------------

base_config = hancho.config.extend(
  name        = "base_config",
  build_type  = "debug",
)

if flags.release:
  base_config.build_type = "release"

#-------------------------------------------------------------------------------

compile_cpp = base_config.extend(
  description = "Compiling {files_in[0]} -> {files_out[0]} ({build_type})",
  command     = "{toolchain}-g++ {cpp_std} {gcc_opt} {warnings} {includes} {defines} -c {files_in[0]} -o {files_out[0]}",

  toolchain   = "x86_64-linux-gnu",
  cpp_std     = "-std=c++20",
  gcc_opt     = "{'-O3' if build_type == 'release' else '-g -O0'} -MMD",
  warnings    = "-Wall -Werror -Wno-unused-variable -Wno-unused-local-typedefs -Wno-unused-but-set-variable",
  includes    = "-I.",
  defines     = "",
  files_out   = "{swap_ext(files_in[0], '.o')}",
  depfile     = "{swap_ext(files_in[0], '.d')}",
)

link_c_lib = base_config.extend(
  description = "Bundling {files_out[0]}",
  command     = "ar rcs {files_out[0]} {join(files_in)}",
)

link_c_bin = base_config.extend(
  description = "Linking {files_out[0]}",
  command     = "{toolchain}-g++ {ld_opt} {warnings} {join(files_in)} {join(deps)} {sys_libs} -o {files_out[0]}",

  toolchain   = "x86_64-linux-gnu",
  ld_opt      = "{'-O3' if build_type == 'release' else '-g -O0'}",
  warnings    = "-Wall",
  sys_libs    = "",
)

test_rule = base_config.extend(
  description = "Running test {files_in[0]}",
  command     = "rm -f {files_out[0]} && {files_in[0]} {args} && touch {files_out[0]}",
  files_out   = ["{files_in[0]}_pass"],
  args        = "",
)

def run_test(*, name, srcs, **kwargs):
  test_bin = c_binary(name = name, srcs = srcs, **kwargs)
  test_rule(files_in = [test_bin], **kwargs)

#-------------------------------------------------------------------------------

def compile_srcs(srcs, **kwargs):
  return [compile_cpp(files_in = [f], **kwargs) for f in srcs]

#-------------------------------------------------------------------------------

def c_binary(*, name, srcs, **kwargs):
  return link_c_bin(
    files_in  = compile_srcs(srcs, **kwargs),
    files_out = [name],
    **kwargs
  )

#-------------------------------------------------------------------------------

def c_library(*, name, srcs, **kwargs):
  return link_c_lib(
    files_in  = compile_srcs(srcs, **kwargs),
    files_out = [name],
    **kwargs)

#-------------------------------------------------------------------------------
