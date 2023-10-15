//------------------------------------------------------------------------------
// This file is a full working example of using Matcheroni to build a JSON
// parser.

// Example usage:
// bin/json_parser test_file.json

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "json.hpp"
#include "matcheroni/Utilities.hpp"

#include <stdio.h>

using namespace matcheroni;
using namespace parseroni;

//------------------------------------------------------------------------------

int main(int argc, char** argv) {

  if (argc < 2) {
    printf("Usage: json_demo <filename>\n");
    return 1;
  }

  printf("Parsing %s\n", argv[1]);
  printf("\n");

  char* buf = nullptr;
  size_t size = 0;
  utils::read(argv[1], buf, size);
  if (size == 0) {
    printf("Could not load %s\n", argv[1]);
    exit(-1);
  }


  //----------------------------------------

  JsonParseContext ctx;
  double time = -utils::timestamp_ms();
  TextSpan text = {buf, buf + size};
  TextSpan parse_end = parse_json(ctx, text);
  time += utils::timestamp_ms();

  if (parse_end.begin < text.end) {
    printf("Parse failed!\n");
    printf("Failure near `");
    printf("`\n");
    exit(-1);
  }

  //----------------------------------------

  utils::print_trees(ctx, text, 40, 0);
  printf("\n");

  printf("Size  %ld bytes\n", size);
  printf("Time  %f msec\n", time);
  printf("Rate  %f mb/sec\n", (size / 1000000.0) / (time / 1000.0));
  printf("\n");

  delete [] buf;
  return 0;
}

//------------------------------------------------------------------------------
