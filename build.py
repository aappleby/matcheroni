#!/usr/bin/python3
# Experimental use of hancho.py, beware

from rules import *

#-fsanitize=undefined

#-------------------------------------------------------------------------------
# Tests

#build obj/matcheroni/Matcheroni.hpp.iwyu : iwyu matcheroni/Matcheroni.hpp
#build obj/matcheroni/Parseroni.hpp.iwyu  : iwyu matcheroni/Parseroni.hpp
#build obj/matcheroni/Utilities.hpp.iwyu  : iwyu matcheroni/Utilities.hpp

c_binary(
  name = "tests/matcheroni_test",
  srcs = ["tests/matcheroni_test.cpp"],
)

c_binary(
  name = "tests/parseroni_test",
  srcs = ["tests/parseroni_test.cpp"],
)

run_test("tests/matcheroni_test", quiet = True)
run_test("tests/parseroni_test", quiet = True)

#-------------------------------------------------------------------------------
# These are the various regex libraries that Matcheroni can be benchmarked
# against. CTRE and SRELL require that you copy their header into matcheroni/.

# These defines are required to reduce the compiled size of the SRELL library used in the benchmark.
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_UNICODE_ICASE
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_UNICODE_PROPERTY
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_UNICODE_DATA
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_NAMEDCAPTURE
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_VMODE

c_library(
  name = "examples/regex/regex_parser.a",
  srcs = ["examples/regex/regex_parser.cpp"],
)

c_binary(
  name = "examples/regex/regex_demo",
  srcs = ["examples/regex/regex_demo.cpp"],
  deps = ["examples/regex/regex_parser.a"],
)

c_binary(
  name = "examples/regex/regex_benchmark",
  srcs = ["examples/regex/regex_benchmark.cpp"],
  deps = ["examples/regex/regex_parser.a"],
  libs = "-lboost_system -lboost_regex",
)

c_binary(
  name = "examples/regex/regex_test",
  srcs = ["examples/regex/regex_test.cpp"],
  deps = ["examples/regex/regex_parser.a"],
)

run_test("examples/regex/regex_test", quiet = True)

#-------------------------------------------------------------------------------
# INI parser example

c_library(
  name = "ini_parser",
  srcs = ["examples/ini/ini_parser.cpp"],
)

#-------------------------------------------------------------------------------
# TOML parser example

c_binary(
  name = "examples/toml/toml_test",
  srcs = ["examples/toml/toml_parser.cpp", "examples/toml/toml_test.cpp"]
)

#run_test("examples/toml/toml_test")

#-------------------------------------------------------------------------------
# JSON parser example

c_library(
  name = "examples/json/json_parser.a",
  srcs = ["examples/json/json_matcher.cpp", "examples/json/json_parser.cpp"]
)

c_binary(
  name = "examples/json/json_conformance",
  srcs = ["examples/json/json_conformance.cpp"],
  deps = ["examples/json/json_parser.a"]
)

c_binary(
  name = "examples/json/json_benchmark",
  srcs = ["examples/json/json_benchmark.cpp"],
  deps = ["examples/json/json_parser.a"]
)

c_binary(
  name = "examples/json/json_demo",
  srcs = ["examples/json/json_demo.cpp"],
  deps = ["examples/json/json_parser.a"],
)

c_binary(
  name = "examples/json/json_test",
  srcs = ["examples/json/json_test.cpp"],
  deps = ["examples/json/json_parser.a"]
)

run_test("examples/json/json_test", quiet = True)

#-------------------------------------------------------------------------------
# Tutorial examples

c_binary(name = "tutorial/json_tut0a", srcs = ["tutorial/json_tut0a.cpp"])
c_binary(name = "tutorial/json_tut1a", srcs = ["tutorial/json_tut1a.cpp"])
c_binary(name = "tutorial/json_tut1b", srcs = ["tutorial/json_tut1b.cpp"])
c_binary(name = "tutorial/json_tut1c", srcs = ["tutorial/json_tut1c.cpp"])
c_binary(name = "tutorial/json_tut2a", srcs = ["tutorial/json_tut2a.cpp"])
c_binary(name = "tutorial/json_tut2b", srcs = ["tutorial/json_tut2b.cpp"])

run_test("tutorial/json_tut0a", quiet = True)
run_test("tutorial/json_tut1a", quiet = True)
run_test("tutorial/json_tut1b", quiet = True)
run_test("tutorial/json_tut1c", quiet = True)
run_test("tutorial/json_tut2a", quiet = True)
run_test("tutorial/json_tut2b", quiet = True)

c_binary(
  name = "tutorial/tiny_c_parser",
  srcs = ["tutorial/tiny_c_parser.cpp"],
  deps = ["examples/c_lexer.a", "examples/c_parser.a"]
)

#-------------------------------------------------------------------------------
# C lexer example (not finished)

c_library(
  name = "examples/c_lexer.a",
  srcs = ["examples/c_lexer/CLexer.cpp", "examples/c_lexer/CToken.cpp"]
)

c_binary(
  name = "examples/c_lexer_benchmark",
  srcs = ["examples/c_lexer/c_lexer_benchmark.cpp"],
  deps = ["examples/c_lexer.a"]
)

c_binary(
  name = "examples/c_lexer_test",
  srcs = ["examples/c_lexer/c_lexer_test.cpp"],
  deps = ["examples/c_lexer.a"]
)

run_test("examples/c_lexer_test", quiet = True)

#-------------------------------------------------------------------------------
# C parser example (not finished)

c_library(
  name = "examples/c_parser.a",
  srcs = [
    "examples/c_parser/CContext.cpp",
    "examples/c_parser/CNode.cpp",
    "examples/c_parser/CScope.cpp"
  ]
)

c_binary(
  name = "examples/c_parser_benchmark",
  srcs = ["examples/c_parser/c_parser_benchmark.cpp"],
  deps = ["examples/c_lexer.a", "examples/c_parser.a"]
)

c_binary(
  name = "examples/c_parser_test",
  srcs = ["examples/c_parser/c_parser_test.cpp"],
  deps = ["examples/c_lexer.a", "examples/c_parser.a"]
)

# Broken?
#run_test("examples/c_parser_test")

#-------------------------------------------------------------------------------

hancho.build()
