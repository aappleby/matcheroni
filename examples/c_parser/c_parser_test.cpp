// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include <filesystem>
#include <stdio.h>

#include "matcheroni/Utilities.hpp"

#include "examples/c_lexer/CLexer.hpp"
#include "examples/c_parser/CContext.hpp"
#include "examples/c_parser/CNode.hpp"

using namespace matcheroni;

//------------------------------------------------------------------------------

const char* source = R"(
#include <stdio.h>

namespace {
struct foo {
  int x;
  int y;
  void blep() {}
};
};

int main(int argc, char** argv) {
  printf("Hello World\n");
  int x = florble(+1++,2,3);
  int y = +a-- + -b++;
  return 0;
}
)";

//------------------------------------------------------------------------------

int test_parser() {
  CLexer lexer;
  CContext context;

  lexer.reset();
  context.reset();

  auto span = to_span(source);

  if (lexer.lex(span)) {
    printf("Lex OK!\n");
  }
  else {
    printf("Lex failed!\n");
  }

  if (context.parse(lexer.tokens)) {
    printf("Parse OK!\n");
  }
  else {
    printf("Parse failed!\n");
  }

  printf("Dumping tree:\n");
  print_context(span, context, 40);

  printf("Node max size  %d bytes\n", LinearAlloc::inst().max_size);
  printf("Node count     %ld\n", context.node_count());
  printf("Node max count %ld\n", LinearAlloc::inst().max_size / sizeof(CNode));

  return 0;
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("c_parser_test\n");
  test_parser();
  printf("c_parser_test done\n");
  return 0;
}

//------------------------------------------------------------------------------
