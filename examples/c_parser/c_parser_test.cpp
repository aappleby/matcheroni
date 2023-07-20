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
typedef __SIZE_TYPE__ size_t;
)";

//------------------------------------------------------------------------------

int test_parser() {
  CLexer lexer;
  CContext context;

  lexer.reset();
  context.reset();

  auto text_span = to_span(source);
  auto lex_ok = lexer.lex(text_span);
  printf("Lex %s!\n", lex_ok ? "OK" : "failed");

  TokSpan tok_span = to_span(lexer.tokens);
  auto parse_ok = context.parse(text_span, tok_span);
  printf("Parse %s!\n", parse_ok ? "OK" : "failed");

  printf("Dumping tree:\n");
  print_context(text_span, context, 40);

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
