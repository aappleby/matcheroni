// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c_lexer/CLexer.hpp"
#include "examples/c_parser/CContext.hpp"
#include "examples/c_parser/CNode.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

int main(int argc, char** argv) {
  const char* filename = argc < 2 ? "examples/tutorial/tiny_c_parser.input" : argv[1];

  std::string input = utils::read(filename);
  auto text = utils::to_span(input);

  CLexer lexer;
  lexer.lex(text);

  CContext context;
  TokenSpan tok_span(lexer.tokens.data(), lexer.tokens.data() + lexer.tokens.size());
  bool parse_ok = context.parse(text, tok_span);

  for (auto n = context.top_head; n; n = n->node_next) {
    utils::print_tree(text, n, 40, 0);
  }
}
