//------------------------------------------------------------------------------
// This file is a full working example of using Matcheroni to build a JSON
// parser.

// Example usage:
// bin/json_parser test_file.json

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

#include <stdio.h>

using namespace matcheroni;

TextSpan match_toml(TextNodeContext& ctx, TextSpan text);

//------------------------------------------------------------------------------

const char* toml = R"(
[workspace]
resolver = "2"
[target.'cfg(not(windows))'.dependencies]
description = """
Cargo, a package manager for Rust.
"""
members = [
  "crates/*",
  "credential/*",
  "benches/benchsuite",
  "benches/capture",
]
exclude = [
  "target/",
]
)";

int main(int argc, char** argv) {
  /*
  if (argc < 2) {
    printf("Usage: toml_test <filename>\n");
    return 1;
  }

  printf("argv[0] = %s\n", argv[0]);
  printf("argv[1] = %s\n", argv[1]);

  std::string buf;
  read(argv[1], buf);

  TextSpan text = to_span(buf);
  */

  TextSpan text = to_span(toml);
  TextNodeContext ctx;

  double time_a, time_b;
  TextSpan tail;

  for (int rep = 0; rep < 100; rep++) {
    ctx.reset();
    time_a = timestamp_ms();
    tail = match_toml(ctx, text);
    time_b = timestamp_ms();
  }

  printf("Parsing toml took %f msec\n", time_b - time_a);
  print_summary(text, tail, ctx, 40);

  return 0;
}

//------------------------------------------------------------------------------
