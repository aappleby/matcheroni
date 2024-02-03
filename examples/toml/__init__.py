#-------------------------------------------------------------------------------
# TOML parser example

import rules

import os
old_dir = os.getcwd()
os.chdir(os.path.split(__file__)[0])

toml_test = rules.c_binary(name = "toml_test", srcs = ["toml_parser.cpp", "toml_test.cpp"])

rules.test_rule(files_in = [toml_test], quiet = True)

os.chdir(old_dir)
