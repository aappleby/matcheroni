// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c99_parser/Token.hpp"
#include "examples/c99_parser/Lexeme.hpp"

using namespace matcheroni;

//----------------------------------------------------------------------------

Token::Token(const Lexeme* lex) {
  this->lex = lex;
}

//----------------------------------------------------------------------------

void Token::dump_token() const {
#if 0
  // Dump token
  printf("tok @ %p :", this);

  printf(" %14.14s ", type_to_str());
  set_color(type_to_color());
  lex->dump_lexeme();
  set_color(0);

  printf("    span %14p ", span);
  if (span) {
    printf("{");
    span->print_class_name(20);
    printf("} prec %d", span->precedence);
  } else {
    printf("{                    }");
  }
  printf("\n");
#endif
}

//------------------------------------------------------------------------------
