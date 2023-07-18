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
int main(int argc, char** argv) {
  printf("Hello World\n");
  return 0;
}
)";

/*
#include <stdio.h>
int main(int argc, char** argv) {
  printf("Hello World\n");
  return 0;
}
*/

//------------------------------------------------------------------------------

int test_parser() {
  printf("Matcheroni c_parser_test\n");

  CLexer lexer;
  CContext context;

  lexer.reset();
  context.reset();

  if (lexer.lex(to_span(source))) {
    printf("Lex OK!\n");
  }
  else {
    printf("Lex failed!\n");
  }

  //lexer.dump_lexemes();

  if (context.parse(lexer.tokens)) {
    printf("Parse OK!\n");
  }
  else {
    printf("Parse failed!\n");
  }

  //context.dump_tokens();

  printf("Dumping tree:\n");
  for (auto c = context.top_head(); c; c = c->node_next()) {
    c->dump_tree(0, 0);
  }

  //printf("Total nodes    %ld\n",      CNode::constructor_calls);
  printf("Node pool      %d bytes\n", LinearAlloc::inst().max_size);

  return 0;
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) { test_parser(); }

//------------------------------------------------------------------------------
