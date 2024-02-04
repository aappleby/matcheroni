#-------------------------------------------------------------------------------
# INI parser example

import os, rules

old_dir = os.getcwd()
os.chdir(os.path.split(__file__)[0])

ini_parser = rules.c_library(name = "ini_parser", srcs = ["ini_parser.cpp"])

os.chdir(old_dir)
