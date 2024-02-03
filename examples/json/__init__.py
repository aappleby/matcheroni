#-------------------------------------------------------------------------------
# Matcheroni JSON parser example

import rules

import os
old_dir = os.getcwd()
os.chdir(os.path.split(__file__)[0])

json_parser = rules.c_library(
  name = "json_parser.a",
  srcs = ["json_matcher.cpp", "json_parser.cpp"]
)

json_conformance = rules.c_binary(
  name = "json_conformance",
  srcs = ["json_conformance.cpp"],
  deps = [json_parser]
)

json_benchmark = rules.c_binary(
  name = "json_benchmark",
  srcs = ["json_benchmark.cpp"],
  deps = [json_parser]
)

json_demo = rules.c_binary(
  name = "json_demo",
  srcs = ["json_demo.cpp"],
  deps = [json_parser],
)

json_test = rules.c_binary(
  name = "json_test",
  srcs = ["json_test.cpp"],
  deps = [json_parser]
)

rules.test_rule(files_in = [json_test], quiet = True)

os.chdir(old_dir)
