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

cspan parse_regex(void* ctx, cspan s);

//------------------------------------------------------------------------------
// The demo app accepts a quoted regex as its first command line argument,
// attempts to parse it, and then prints out the resulting parse tree.

int main(int argc, char** argv) {

  if (argc < 2) {
    printf("Usage: regex_demo \"<your regex>\"\n");
    return 1;
  }

  // Bash will un-quote the regex on the command line for us, so we don't need
  // to do any processing here.
  cspan span = {argv[1], argv[1] + strlen(argv[1])};

  // Invoke our regex matcher against the input text. If it matches, we will
  // get a non-null endpoint for the match.

  printf("Parsing regex `%s`\n", span.a);
  Context<TextNode>* context = new Context<TextNode>();
  auto parse_end = parse_regex(context, span);

  if (parse_end.a == nullptr) {
    printf("Parse fail at  : %ld\n", parse_end.b - span.a);
    printf("Parse context  : `%-20.20s`\n", parse_end.b);
  }
  else {
    printf("Parse tail len : %ld\n", parse_end.b - parse_end.a);
    printf("Parse tail     : `%s`\n", parse_end.a);
  }
  printf("\n");

  printf("Parse tree:\n");
  printf("----------------------------------------\n");
  for (auto n = context->top_head(); n; n = n->node_next()) {
    print_tree((TextNode*)n);
  }
  printf("----------------------------------------\n");
  printf("\n");

  return 0;
}

//------------------------------------------------------------------------------
