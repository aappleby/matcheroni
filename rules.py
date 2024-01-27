import sys
import argparse
from os import path

sys.path.append("symlinks/hancho")
import hancho

from hancho import Config

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
  out_dir     = "build/{build_type}",
)

if flags.release:
  base_config.build_type = "release"

#-------------------------------------------------------------------------------

rule_compile_cpp = base_config.extend(
  name        = "rule_compile_cpp",
  description = "Compiling {file_in} -> {file_out} ({build_type})",
  command     = "{toolchain}-g++ {cpp_std} {build_opt} {warnings} {depfile} {includes} {defines} -c {file_in} -o {file_out}",

  toolchain   = "x86_64-linux-gnu",
  cpp_std     = "-std=c++20",
  build_opt   = "{'-O3' if build_type == 'release' else '-g -O0'}",
  warnings    = "-Wall -Werror -Wno-unused-variable -Wno-unused-local-typedefs -Wno-unused-but-set-variable",
  depfile     = "-MMD -MF {file_out}.d",
  includes    = "-I.",
  defines     = "",
  file_out    = "{out_dir}/{swap_ext(file_in, '.o')}",
)

link_c_lib = base_config.extend(
  name        = "link_c_lib",
  description = "Bundling {file_out}",
  command     = "ar rcs {file_out} {join(files_in)}",
)

link_c_bin = base_config.extend(
  name        = "link_c_bin",
  description = "Linking {file_out}",
  command     = "{toolchain}-g++ {build_opt} {warnings} {join(files_in)} {join(deps)} {sys_libs} -o {file_out}",
  build_opt   = "{'-O3' if build_type == 'release' else '-g -O0'}",
  toolchain   = "x86_64-linux-gnu",
  warnings    = "-Wall",
  sys_libs    = "",
)

test_rule = base_config.extend(
  name        = "test_rule",
  description = "Running test {file_in}",
  command     = "rm -f {file_out} && {file_in} {args} && touch {file_out}",
  file_out    = "{file_in}_pass",
  args        = "",
)

#-------------------------------------------------------------------------------

def run_test(name, **kwargs):
  return test_rule(file_in = name, **kwargs)

#-------------------------------------------------------------------------------

def compile_srcs(srcs, **kwargs):
  objs = []
  for src_file in srcs:
    result = rule_compile_cpp(file_in = src_file, **kwargs)
    objs.append(result)
  return objs

#-------------------------------------------------------------------------------

def c_binary(*, name, srcs, **kwargs):
  return link_c_bin(
    files_in = compile_srcs(srcs, **kwargs),
    file_out = "{out_dir}/" + name,
    **kwargs
  )

#-------------------------------------------------------------------------------

def c_library(*, name, srcs, **kwargs):
  return link_c_lib(
    files_in = compile_srcs(srcs, **kwargs),
    file_out = "{out_dir}/" + name,
    **kwargs)

#-------------------------------------------------------------------------------
