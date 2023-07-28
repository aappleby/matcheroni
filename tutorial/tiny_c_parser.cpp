// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c_lexer/CLexer.hpp"
#include "examples/c_parser/CContext.hpp"
#include "examples/c_parser/CNode.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

int main(int argc, char** argv) {
  if (argc < 2) exit(-1);

  std::string text = utils::read(argv[1]);
  auto body = utils::to_span(text);

  CLexer lexer;
  lexer.lex(body);

  CContext context;
  TokenSpan tok_span(lexer.tokens.data(), lexer.tokens.data() + lexer.tokens.size());
  bool parse_ok = context.parse(body, tok_span);

  for (auto n = context.top_head; n; n = n->node_next) {
    utils::print_tree(body, n, 40, 0);
  }
}
