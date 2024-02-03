#-------------------------------------------------------------------------------
# Tests

import rules

import os
old_dir = os.getcwd()
os.chdir(os.path.split(__file__)[0])

#build obj/matcheroni/Matcheroni.hpp.iwyu : iwyu matcheroni/Matcheroni.hpp
#build obj/matcheroni/Parseroni.hpp.iwyu  : iwyu matcheroni/Parseroni.hpp
#build obj/matcheroni/Utilities.hpp.iwyu  : iwyu matcheroni/Utilities.hpp

matcheroni_test = rules.c_binary(
  name = "matcheroni_test",
  srcs = ["matcheroni_test.cpp"],
)

parseroni_test = rules.c_binary(
  name = "parseroni_test",
  srcs = ["parseroni_test.cpp"],
)

rules.test_rule(files_in = matcheroni_test, quiet = True)
rules.test_rule(files_in = parseroni_test, quiet = True)

os.chdir(old_dir)