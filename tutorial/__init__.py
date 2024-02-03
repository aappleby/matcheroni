#-------------------------------------------------------------------------------
# Tutorial examples

import rules
import examples.c_lexer
import examples.c_parser

import os
old_dir = os.getcwd()
os.chdir(os.path.split(__file__)[0])

json_tut0a = rules.c_binary(name = "json_tut0a", srcs = ["json_tut0a.cpp"])
json_tut1a = rules.c_binary(name = "json_tut1a", srcs = ["json_tut1a.cpp"])
json_tut1b = rules.c_binary(name = "json_tut1b", srcs = ["json_tut1b.cpp"])
json_tut1c = rules.c_binary(name = "json_tut1c", srcs = ["json_tut1c.cpp"])
json_tut2a = rules.c_binary(name = "json_tut2a", srcs = ["json_tut2a.cpp"])
json_tut2b = rules.c_binary(name = "json_tut2b", srcs = ["json_tut2b.cpp"])

rules.test_rule(files_in = json_tut0a, quiet = True)
rules.test_rule(files_in = json_tut1a, quiet = True)
rules.test_rule(files_in = json_tut1b, quiet = True)
rules.test_rule(files_in = json_tut1c, quiet = True)
rules.test_rule(files_in = json_tut2a, quiet = True)
rules.test_rule(files_in = json_tut2b, quiet = True)

tiny_c_parser = rules.c_binary(
  name = "tiny_c_parser",
  srcs = ["tiny_c_parser.cpp"],
  deps = [examples.c_lexer.c_lexer, examples.c_parser.c_parser],
  #deps = ["examples.c_lexer", "examples.c_parser"],
)

os.chdir(old_dir)
