//------------------------------------------------------------------------------
// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "json_parser.hpp"
#include "matcheroni/Utilities.hpp"

#include <stdio.h>

using namespace matcheroni;

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

  TextSpan text = utils::to_span(json);
  JsonContext ctx;

  double time_a, time_b;
  TextSpan tail;

  for (int rep = 0; rep < 100; rep++) {
    ctx.reset();
    time_a = utils::timestamp_ms();
    tail = parse_json(ctx, text);
    time_b = utils::timestamp_ms();
  }

  printf("Parsing json took %f msec\n", time_b - time_a);
  utils::print_summary(ctx, text, tail, 50);

  return 0;
}

//------------------------------------------------------------------------------
