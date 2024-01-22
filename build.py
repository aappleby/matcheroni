#!/usr/bin/python3
# Experimental use of hancho.py, beware

import argparse
import glob
import os
import pprint
import sys

sys.path.append("symlinks/hancho")
import hancho

from hancho import Config

pp = pprint.PrettyPrinter(depth=6)

#-fsanitize=undefined

################################################################################

parser = argparse.ArgumentParser()
parser.add_argument('--verbose',  default=False, action='store_true', help='Print verbose build info')
parser.add_argument('--clean',    default=False, action='store_true', help='Delete intermediate files')
parser.add_argument('--serial',   default=False, action='store_true', help='Do not parallelize commands')
parser.add_argument('--dry_run',  default=False, action='store_true', help='Do not run commands')
parser.add_argument('--debug',    default=False, action='store_true', help='Dump debugging information')
parser.add_argument('--dotty',    default=False, action='store_true', help='Dump dependency graph as dotty')
parser.add_argument('--release',  default=False, action='store_true', help='Release-mode optimized build')
(flags, unrecognized) = parser.parse_known_args()

hancho.config.verbose    = flags.verbose
hancho.config.clean      = flags.clean
hancho.config.serial     = flags.serial
hancho.config.dry_run    = flags.dry_run
hancho.config.debug      = flags.debug
hancho.config.dotty      = flags.dotty

################################################################################

# Turning tracing on will generate _ton_ of spam in the C parser demo.
# Turning EXTRA_DEBUG on will generate even more spam.

#defines = -DMATCHERONI_ENABLE_TRACE
#defines = -DEXTRA_DEBUG

compile_cpp = Config(
  prototype   = hancho.config,
  description = "Compiling {file_in} -> {file_out} ({build_type})",
  command     = "{toolchain}-g++ {build_opt} {cpp_std} {warnings} {depfile} {includes} {defines} -c {file_in} -o {file_out}",
  build_type  = "debug",
  build_opt   = "-g -O0",
  toolchain   = "x86_64-linux-gnu",
  cpp_std     = "-std=c++20",
  warnings    = "-Wall -Werror -Wno-unused-variable -Wno-unused-local-typedefs -Wno-unused-but-set-variable",
  depfile     = "-MMD -MF {file_out}.d",
  includes    = "-I.",
  defines     = "",
)

if flags.release:
  compile_cpp.build_type = "release"
  compile_cpp.build_opt  = "-O3"

#-------------------------------------------------------------------------------

link_c_lib = Config(
  prototype   = hancho.config,
  description = "Bundling {file_out}",
  command     = "ar rcs {file_out} {join(files_in)}",
)

link_c_bin = Config(
  prototype   = hancho.config,
  description = "Linking {file_out}",
  command     = "{toolchain}-g++ {join(files_in)} {join(deps)} {libraries} -o {file_out}",
  toolchain   = "x86_64-linux-gnu",
  libraries   = ""
)

#-------------------------------------------------------------------------------

def run_test(name, **kwargs):
  my_config = Config(
    prototype   = hancho.config,
    description = "Running test {file_in}",
    command     = "rm -f {file_out} && {file_in} {args} && touch {file_out}",
    args        = "",
  )
  bin_name = os.path.join("bin", name)
  my_config(
    files_in  = [bin_name],
    files_out = [bin_name + "_pass"],
    deps      = [],
    **kwargs
  )

#-------------------------------------------------------------------------------

def obj_name(x):
  return "obj/" + hancho.swap_ext(x, ".o")

def compile_dir(dir, **kwargs):
  files = glob.glob(dir + "/*.cpp") + glob.glob(dir + "/*.c")
  objs = []
  for file in files:
    obj = obj_name(file)
    compile_cpp(
      files_in  = [file],
      files_out = [obj],
      deps      = [],
      **kwargs)
    objs.append(obj)
  return objs

def c_binary(*, name, srcs, deps, **kwargs):
  objs = []
  for src_file in srcs:
    obj_file = os.path.join("obj", os.path.splitext(src_file)[0] + ".o")
    compile_cpp(
      files_in = [src_file],
      files_out = [obj_file],
      deps = deps,
      **kwargs)
    objs.append(obj_file)
  bin_name = os.path.join("bin", name)
  link_c_bin(files_in = objs, files_out = [bin_name], deps = [], **kwargs)

#-------------------------------------------------------------------------------
# Tests

#build obj/matcheroni/Matcheroni.hpp.iwyu : iwyu matcheroni/Matcheroni.hpp
#build obj/matcheroni/Parseroni.hpp.iwyu  : iwyu matcheroni/Parseroni.hpp
#build obj/matcheroni/Utilities.hpp.iwyu  : iwyu matcheroni/Utilities.hpp

c_binary(
  name = "tests/matcheroni_test",
  srcs = ["tests/matcheroni_test.cpp"],
  deps = [],
)

c_binary(
  name = "tests/parseroni_test",
  srcs = ["tests/parseroni_test.cpp"],
  deps = [],
)

run_test("tests/matcheroni_test")
run_test("tests/parseroni_test")

#-------------------------------------------------------------------------------
# These are the various regex libraries that Matcheroni can be benchmarked
# against. CTRE and SRELL require that you copy their header into matcheroni/.

# These defines are required to reduce the compiled size of the SRELL library used in the benchmark.
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_UNICODE_ICASE
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_UNICODE_PROPERTY
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_UNICODE_DATA
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_NAMEDCAPTURE
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_VMODE

c_binary(
  name = "examples/regex/regex_demo",
  srcs = ["examples/regex/regex_parser.cpp", "examples/regex/regex_demo.cpp"],
  deps = [],
)

c_binary(
  name = "examples/regex/regex_benchmark",
  srcs = ["examples/regex/regex_parser.cpp", "examples/regex/regex_benchmark.cpp"],
  deps = [],
  libraries = "-lboost_system -lboost_regex",
)

"""

#run_test("bin/examples/regex/regex_demo", "bin/examples/regex/regex_demo_pass", args = "a*")
#run_test("bin/examples/regex/regex_test", "bin/examples/regex/regex_test_pass", args = "a*")

#-------------------------------------------------------------------------------
# INI parser example

compile_cpp("examples/ini/ini_parser.cpp", "obj/examples/ini/ini_parser.o")

#-------------------------------------------------------------------------------
# TOML parser example

compile_cpp("examples/toml/toml_parser.cpp", "obj/examples/toml/toml_parser.o")

compile_cpp("examples/toml/toml_test.cpp",   "obj/examples/toml/toml_test.o")
link_c_bin(["obj/examples/toml/toml_parser.o", "obj/examples/toml/toml_test.o"], "bin/examples/toml/toml_test")
#run_test("bin/examples/toml/toml_test", "bin/examples/toml/toml_test_pass")

#-------------------------------------------------------------------------------
# JSON parser example

compile_cpp("examples/json/json_matcher.cpp",     "obj/examples/json/json_matcher.o")
compile_cpp("examples/json/json_parser.cpp",      "obj/examples/json/json_parser.o")
compile_cpp("examples/json/json_conformance.cpp", "obj/examples/json/json_conformance.o")

link_c_bin(
  [
    "obj/examples/json/json_matcher.o",
    "obj/examples/json/json_parser.o",
    "obj/examples/json/json_conformance.o"
  ],
  "bin/examples/json/json_conformance"
)

compile_cpp("examples/json/json_benchmark.cpp", "obj/examples/json/json_benchmark.o")

link_c_bin(
  [
    "obj/examples/json/json_matcher.o",
    "obj/examples/json/json_parser.o",
    "obj/examples/json/json_benchmark.o"
  ],
  "bin/examples/json/json_benchmark"
)

compile_cpp("examples/json/json_demo.cpp", "obj/examples/json/json_demo.o")

link_c_bin(
  ["obj/examples/json/json_parser.o", "obj/examples/json/json_demo.o"],
  "bin/examples/json/json_demo"
)

compile_cpp("examples/json/json_test.cpp", "obj/examples/json/json_test.o")

link_c_bin(
  ["obj/examples/json/json_parser.o", "obj/examples/json/json_test.o"],
  "bin/examples/json/json_test"
)

#run_test("bin/examples/json/json_test", "bin/examples/json/json_test_pass")

#-------------------------------------------------------------------------------
# Tutorial examples

compile_dir("tutorial")

link_c_bin(
  ["obj/tutorial/tiny_c_parser.o", "obj/examples/c_lexer.a", "obj/examples/c_parser.a"],
  "bin/tutorial/tiny_c_parser"
)

#build bin/tutorial/tiny_c_parser      : c_binary
##build bin/tutorial/tiny_c_parser_pass : run_test bin/tutorial/tiny_c_parser
##  args = tutorial/tiny_c_parser.input

link_c_bin("obj/tutorial/json_tut0a.o", "bin/tutorial/json_tut0a")
link_c_bin("obj/tutorial/json_tut1a.o", "bin/tutorial/json_tut1a")
link_c_bin("obj/tutorial/json_tut1b.o", "bin/tutorial/json_tut1b")
link_c_bin("obj/tutorial/json_tut1c.o", "bin/tutorial/json_tut1c")
link_c_bin("obj/tutorial/json_tut2a.o", "bin/tutorial/json_tut2a")
link_c_bin("obj/tutorial/json_tut2b.o", "bin/tutorial/json_tut2b")

##build bin/tutorial/json_tut0a_pass : run_test bin/tutorial/json_tut0a
##  args = tutorial/json_tut0a.input
#
##build bin/tutorial/json_tut1a_pass : run_test bin/tutorial/json_tut1a
##  args = tutorial/json_tut1a.input
#
##build bin/tutorial/json_tut1b_pass : run_test bin/tutorial/json_tut1b
##  args = tutorial/json_tut1b.input
#
##build bin/tutorial/json_tut1c_pass : run_test bin/tutorial/json_tut1c
##  args = tutorial/json_tut1c.input
#
##build bin/tutorial/json_tut2a_pass : run_test bin/tutorial/json_tut2a
##  args = tutorial/json_tut2a.input
#
##build bin/tutorial/json_tut2b_pass : run_test bin/tutorial/json_tut2b
##  args = tutorial/json_tut2b.input

#-------------------------------------------------------------------------------
# C lexer example (not finished)

compile_dir("examples/c_lexer")
link_c_lib(["obj/examples/c_lexer/CLexer.o", "obj/examples/c_lexer/CToken.o"], "obj/examples/c_lexer.a")

#build bin/examples/c_lexer/c_lexer_benchmark : c_binary obj/examples/c_lexer/c_lexer_benchmark.o obj/examples/c_lexer.a
#build bin/examples/c_lexer/c_lexer_test      : c_binary obj/examples/c_lexer/c_lexer_test.o obj/examples/c_lexer.a
#build bin/examples/c_lexer/c_lexer_test_pass : run_test bin/examples/c_lexer/c_lexer_test

#-------------------------------------------------------------------------------
# C parser example (not finished)

compile_dir("examples/c_parser")

link_c_lib(
  [
    "obj/examples/c_parser/CContext.o",
    "obj/examples/c_parser/CNode.o",
    "obj/examples/c_parser/CScope.o"
  ],
  "obj/examples/c_parser.a"
)

#build bin/examples/c_parser/c_parser_benchmark : c_binary $
#  obj/examples/c_parser/c_parser_benchmark.o $
#  obj/examples/c_lexer.a $
#  obj/examples/c_parser.a
#
#build bin/examples/c_parser/c_parser_test : c_binary $
#  obj/examples/c_parser/c_parser_test.o $
#  obj/examples/c_lexer.a $
#  obj/examples/c_parser.a

#-------------------------------------------------------------------------------
"""
