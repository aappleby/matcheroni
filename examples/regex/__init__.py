#-------------------------------------------------------------------------------
# Matcheroni regex parsing example

import rules

import os
old_dir = os.getcwd()
os.chdir(os.path.split(__file__)[0])

# These are the various regex libraries that Matcheroni can be benchmarked
# against. CTRE and SRELL require that you copy their header into matcheroni/.

# These defines are required to reduce the compiled size of the SRELL library used in the benchmark.
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_UNICODE_ICASE
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_UNICODE_PROPERTY
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_UNICODE_DATA
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_NAMEDCAPTURE
#benchmark_defs = ${benchmark_defs} -DSRELL_NO_VMODE

regex_parser = rules.c_library(
  name = "regex_parser.a",
  srcs = ["regex_parser.cpp"],
)

regex_demo = rules.c_binary(
  name = "regex_demo",
  srcs = ["regex_demo.cpp"],
  deps = [regex_parser],
)

regex_benchmark = rules.c_binary(
  name = "regex_benchmark",
  srcs = ["regex_benchmark.cpp"],
  deps = [regex_parser],
  sys_libs = "-lboost_system -lboost_regex",
)

regex_test = rules.c_binary(
  name = "regex_test",
  srcs = ["regex_test.cpp"],
  deps = [regex_parser],
)

rules.test_rule(files_in = regex_test, quiet = True)

os.chdir(old_dir)