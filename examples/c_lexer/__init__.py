#-------------------------------------------------------------------------------
# Matcheroni C lexer demo

import hancho
import rules

import os
old_dir = os.getcwd()
os.chdir(os.path.split(__file__)[0])

c_lexer = rules.c_library(
  name = "c_lexer.a",
  srcs = ["CLexer.cpp", "CToken.cpp"],
)

c_lexer_benchmark = rules.c_binary(
  name = "c_lexer_benchmark",
  srcs = ["c_lexer_benchmark.cpp"],
  deps = [c_lexer]
)

c_lexer_test = rules.c_binary(
  name = "c_lexer_test",
  srcs = ["c_lexer_test.cpp"],
  deps = [c_lexer]
)

rules.test_rule(files_in = c_lexer_test, quiet = True)

os.chdir(old_dir)
