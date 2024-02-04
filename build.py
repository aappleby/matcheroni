#!/usr/bin/python3

print(__file__)

import sys

import hancho
import rules

tests    = hancho.load_module("tests",    "tests/tests.hancho")
examples = hancho.load_module("examples", "examples/examples.hancho")
tutorial = hancho.load_module("tutorial", "tutorial/tutorial.hancho")

hancho.build()
