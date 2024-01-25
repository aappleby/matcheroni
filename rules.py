import os
import sys
import argparse

sys.path.append("symlinks/hancho")
import hancho

from hancho import Config

#-------------------------------------------------------------------------------

parser = argparse.ArgumentParser()
parser.add_argument('--verbose',  default=False, action='store_true', help='Print verbose build info')
parser.add_argument('--serial',   default=False, action='store_true', help='Do not parallelize commands')
parser.add_argument('--dry_run',  default=False, action='store_true', help='Do not run commands')
parser.add_argument('--debug',    default=False, action='store_true', help='Dump debugging information')
parser.add_argument('--dotty',    default=False, action='store_true', help='Dump dependency graph as dotty')
parser.add_argument('--release',  default=False, action='store_true', help='Release-mode optimized build')
parser.add_argument('--force',    default=False, action='store_true', help='Force rebuild of everything')
(flags, unrecognized) = parser.parse_known_args()

hancho.config.verbose = flags.verbose
hancho.config.serial  = flags.serial
hancho.config.dry_run = flags.dry_run
hancho.config.debug   = flags.debug
hancho.config.dotty   = flags.dotty
hancho.config.force   = flags.force

#-------------------------------------------------------------------------------

base_config = Config(
  prototype   = hancho.config,
  name        = "base_config",
  build_type  = "debug",
  out_dir     = "build/{build_type}",
)

rule_compile_cpp = Config(
  prototype   = base_config,
  name        = "rule_compile_cpp",
  description = "Compiling {file_in} -> {file_out} ({build_type})",
  command     = "{toolchain}-g++ {build_opt} {cpp_std} {warnings} {depfile} {includes} {defines} -c {file_in} -o {file_out}",
  build_opt   = "{'-O3' if build_type == 'release' else '-g -O0'}",
  toolchain   = "x86_64-linux-gnu",
  cpp_std     = "-std=c++20",
  warnings    = "-Wall -Werror -Wno-unused-variable -Wno-unused-local-typedefs -Wno-unused-but-set-variable",
  depfile     = "-MMD -MF {file_out}.d",
  includes    = "-I.",
  defines     = "",
)

if flags.release:
  base_config.build_type = "release"

#-------------------------------------------------------------------------------

link_c_lib = Config(
  name        = "link_c_lib",
  prototype   = base_config,
  description = "Bundling {file_out}",
  command     = "ar rcs {file_out} {join(files_in)}",
)

link_c_bin = Config(
  name        = "link_c_bin",
  prototype   = base_config,
  description = "Linking {file_out}",
  command     = "{toolchain}-g++ {join(files_in)} {join(deps)} {libs} -o {file_out}",
  toolchain   = "x86_64-linux-gnu",
  libs        = ""
)

test_rule = Config(
  name        = "test_rule",
  prototype   = base_config,
  description = "Running test {file_in}",
  command     = "rm -f {file_out} && {file_in} {args} && touch {file_out}",
  args        = "",
)

#-------------------------------------------------------------------------------

def run_test(name, **kwargs):
  bin_name = os.path.join("{out_dir}", name)
  test_rule(files_in  = [bin_name], files_out = [bin_name + "_pass"], **kwargs)

#-------------------------------------------------------------------------------

def compile_srcs(srcs, **kwargs):
  objs = []
  for src_file in srcs:
    obj_file = os.path.splitext(src_file)[0] + ".o"
    obj_file = os.path.join("{out_dir}", obj_file)
    rule_compile_cpp(files_in = [src_file], files_out = [obj_file], **kwargs)
    objs.append(obj_file)
  return objs

#-------------------------------------------------------------------------------

def c_binary(*, name, srcs, deps = [], **kwargs):
  hancho.queue(hancho.Config(
    prototype = link_c_bin,
    files_in  = compile_srcs(srcs, **kwargs),
    files_out = [os.path.join("{out_dir}", name)],
    deps      = [os.path.join("{out_dir}", dep) for dep in deps],
    **kwargs))

#-------------------------------------------------------------------------------

def c_library(*, name, srcs, deps = [], **kwargs):
  objs = compile_srcs(srcs, **kwargs)
  bin_name = os.path.join("{out_dir}", name)
  deps = [os.path.join("{out_dir}", dep) for dep in deps]
  link_c_lib(files_in = objs, files_out = [bin_name], deps = deps, **kwargs)

#-------------------------------------------------------------------------------
