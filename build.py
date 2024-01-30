#!/usr/bin/python3
# Experimental use of hancho.py, beware

import hancho

hancho.load("rules.hancho")

print(c_binary)

hancho.load("examples/c_lexer/hancho")
#hancho.load("tests/build.hancho")


"""
#-------------------------------------------------------------------------------
# These are the various regex libraries that Matcheroni can be benchmarked
# against. CTRE and SRELL require that you copy their header into matcheroni/.

# These defines are required to reduce the compiled size of the SRELL library used in the benchmark.
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_UNICODE_ICASE
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_UNICODE_PROPERTY
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_UNICODE_DATA
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_NAMEDCAPTURE
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_VMODE

with hancho.cwd("examples"):
  with hancho.cwd("regex"):
    regex_parser = c_library(
      name = "regex_parser.a",
      srcs = ["regex_parser.cpp"],
    )

    regex_demo = c_binary(
      name = "regex_demo",
      srcs = ["regex_demo.cpp"],
      deps = [regex_parser],
    )

    regex_benchmark = c_binary(
      name = "regex_benchmark",
      srcs = ["regex_benchmark.cpp"],
      deps = [regex_parser],
      gcc_opt  = compile_cpp.gcc_opt  + " "                              + compile_cpp.cmd('pkg-config --cflags-only-other gtk+-3.0'),
      includes = compile_cpp.includes + " "                              + compile_cpp.cmd('pkg-config --cflags-only-I gtk+-3.0'),
      sys_libs = link_c_bin.sys_libs  + " -lboost_system -lboost_regex " + compile_cpp.cmd('pkg-config --libs gtk+-3.0'),
    )

    regex_test = c_binary(
      name = "regex_test",
      srcs = ["regex_test.cpp"],
      deps = [regex_parser],
    )

    test_rule(file_in = regex_test, quiet = True)

#-------------------------------------------------------------------------------
# INI parser example

with hancho.cwd("ini"):
  ini_parser = c_library(name = "ini_parser", srcs = ["ini_parser.cpp"])

#-------------------------------------------------------------------------------
# TOML parser example

with hancho.cwd("toml"):
  toml_test = c_binary(name = "toml_test", srcs = ["toml_parser.cpp", "toml_test.cpp"])
  test_rule(file_in = toml_test, quiet = True)

#-------------------------------------------------------------------------------
# JSON parser example

with hancho.cwd("json"):
  json_parser = c_library(
    name = "json_parser.a",
    srcs = ["json_matcher.cpp", "json_parser.cpp"]
  )

  json_conformance = c_binary(
    name = "json_conformance",
    srcs = ["json_conformance.cpp"],
    deps = [json_parser]
  )

  json_benchmark = c_binary(
    name = "json_benchmark",
    srcs = ["json_benchmark.cpp"],
    deps = [json_parser]
  )

  json_demo = c_binary(
    name = "json_demo",
    srcs = ["json_demo.cpp"],
    deps = [json_parser],
  )

  json_test = c_binary(
    name = "json_test",
    srcs = ["json_test.cpp"],
    deps = [json_parser]
  )

  test_rule(file_in = json_test, quiet = True)

#-------------------------------------------------------------------------------
# C lexer example (not finished)

with hancho.cwd("c_lexer"):
  c_lexer = c_library(
    name = "c_lexer.a",
    srcs = ["CLexer.cpp", "CToken.cpp"]
  )

  c_lexer_benchmark = c_binary(
    name = "c_lexer_benchmark",
    srcs = ["c_lexer_benchmark.cpp"],
    deps = [c_lexer]
  )

  c_lexer_test = c_binary(
    name = "c_lexer_test",
    srcs = ["c_lexer_test.cpp"],
    deps = [c_lexer]
  )

  test_rule(file_in = c_lexer_test, quiet = True)

#-------------------------------------------------------------------------------
# C parser example (not finished)

with hancho.cwd("c_parser"):
  c_parser = c_library(
    name = "c_parser.a",
    srcs = [
      "CContext.cpp",
      "CNode.cpp",
      "CScope.cpp"
    ]
  )

  c_parser_benchmark = c_binary(
    name = "c_parser_benchmark",
    srcs = ["c_parser_benchmark.cpp"],
    deps = [c_lexer, c_parser]
  )

  c_parser_test = c_binary(
    name = "c_parser_test",
    srcs = ["c_parser_test.cpp"],
    deps = [c_lexer, c_parser]
  )

  # Broken?
  #test_rule(file_in = "examples/c_parser_test", quiet = True)

#-------------------------------------------------------------------------------
# Tutorial examples

with hancho.cwd("tutorial"):
  json_tut0a = c_binary(name = "json_tut0a", srcs = ["json_tut0a.cpp"])
  json_tut1a = c_binary(name = "json_tut1a", srcs = ["json_tut1a.cpp"])
  json_tut1b = c_binary(name = "json_tut1b", srcs = ["json_tut1b.cpp"])
  json_tut1c = c_binary(name = "json_tut1c", srcs = ["json_tut1c.cpp"])
  json_tut2a = c_binary(name = "json_tut2a", srcs = ["json_tut2a.cpp"])
  json_tut2b = c_binary(name = "json_tut2b", srcs = ["json_tut2b.cpp"])

  test_rule(file_in = json_tut0a, quiet = True)
  test_rule(file_in = json_tut1a, quiet = True)
  test_rule(file_in = json_tut1b, quiet = True)
  test_rule(file_in = json_tut1c, quiet = True)
  test_rule(file_in = json_tut2a, quiet = True)
  test_rule(file_in = json_tut2b, quiet = True)

  tiny_c_parser = c_binary(
    name = "tiny_c_parser",
    srcs = ["tiny_c_parser.cpp"],
    deps = [c_lexer, c_parser]
  )


#-------------------------------------------------------------------------------
"""

hancho.build()
