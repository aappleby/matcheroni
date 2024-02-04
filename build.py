#!/usr/bin/python3

import hancho

hancho.load("tests")
hancho.load("examples/c_lexer")
hancho.load("examples/c_parser")
hancho.load("examples/ini")
hancho.load("examples/json")
hancho.load("examples/regex")
hancho.load("examples/toml")
hancho.load("tutorial")

hancho.build()
