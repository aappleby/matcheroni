//------------------------------------------------------------------------------
// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

#include <stdio.h>

using namespace matcheroni;

TextSpan parse_json(TextNodeContext& ctx, TextSpan text);

//------------------------------------------------------------------------------

const char* json = R"(
{
  "asdf" : "slkjdfsldkj"
}
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

  TextSpan text = to_span(json);
  TextNodeContext ctx;

  double time_a, time_b;
  TextSpan tail;

  for (int rep = 0; rep < 100; rep++) {
    ctx.reset();
    time_a = timestamp_ms();
    tail = parse_json(ctx, text);
    time_b = timestamp_ms();
  }

  printf("Parsing toml took %f msec\n", time_b - time_a);
  print_summary(text, tail, ctx, 40);

  return 0;
}

//------------------------------------------------------------------------------
