// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c_parser/CToken.hpp"
#include "examples/c_parser/CLexeme.hpp"

//----------------------------------------------------------------------------

CToken::CToken(const CLexeme* lex) {
  this->lex = lex;
}

//----------------------------------------------------------------------------

#if 0
void CToken::dump_token() const {
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
}
#endif

//------------------------------------------------------------------------------
