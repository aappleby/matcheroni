//------------------------------------------------------------------------------
// This file is a full working example of using Matcheroni to build a parser
// that can parse a subset of regular expressions. Supported operators are
// ^, $, ., *, ?, +, |, (), [], [^], and escaped characters.

// Example usage:
// bin/regex_demo "(^\d+\s+(very)?\s+(good|bad)\s+[a-z]*$)"

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

#include <stdio.h>
#include <string.h>

using namespace matcheroni;
using namespace parseroni;

TextSpan parse_regex(TextParseContext& ctx, TextSpan body);

//------------------------------------------------------------------------------
// The demo app accepts a quoted regex as its first command line argument,
// attempts to parse it, and then prints out the resulting parse tree.

// Bash will un-quote the regex on the command line for us, so we don't need
// to do any processing here.

int main(int argc, char** argv) {
  printf("Regex Demo\n");

  if (argc < 2) {
    printf("Usage: regex_demo \"<your regex>\"\n");
    return 0;
  }

  // Invoke our regex matcher against the input text. If it matches, we will
  // get a non-null endpoint for the match.

  TextParseContext ctx;
  auto text = utils::to_span(argv[1]);
  auto tail = parse_regex(ctx, text);
  utils::print_summary(text, tail, 50);

  return 0;
}

//------------------------------------------------------------------------------
