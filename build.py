#!/usr/bin/python3

import hancho

tests    = hancho.module("tests")
c_lexer  = hancho.module("examples/c_lexer")
c_parser = hancho.module("examples/c_parser")
ini      = hancho.module("examples/ini")
json     = hancho.module("examples/json")
regex    = hancho.module("examples/regex")
toml     = hancho.module("examples/toml")
tutorial = hancho.module("tutorial")

hancho.build()
