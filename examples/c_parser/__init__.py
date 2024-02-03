#-------------------------------------------------------------------------------
# C parser example (not finished)

import os
import rules
import examples.c_lexer

old_dir = os.getcwd()
os.chdir(os.path.split(__file__)[0])

c_parser = rules.c_library(
  name = "c_parser.a",
  srcs = [
    "CContext.cpp",
    "CNode.cpp",
    "CScope.cpp"
  ]
)

c_parser_benchmark = rules.c_binary(
  name = "c_parser_benchmark",
  srcs = ["c_parser_benchmark.cpp"],
  deps = [examples.c_lexer.c_lexer, c_parser]
)

c_parser_test = rules.c_binary(
  name = "c_parser_test",
  srcs = ["c_parser_test.cpp"],
  deps = [examples.c_lexer.c_lexer, c_parser]
)

# Broken?
#test_rule(files_in = [c_parser_test], quiet = True)

os.chdir(old_dir)
