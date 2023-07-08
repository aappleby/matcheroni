// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c99_parser/C99Parser.hpp"

using namespace matcheroni;

//----------------------------------------------------------------------------

Token::Token(const Lexeme* lex) {
  this->lex = lex;
  this->span = nullptr;
}

//----------------------------------------------------------------------------
// These methods REMOVE THE SPAN FROM THE NODE - that's why they're not
// const. This is required to ensure that if an Opt<> matcher fails to match
// a branch, when it tries to match the next branch we will always pull the
// defunct nodes off the tokens.

int Token::atom_cmp(const LexemeType& b) {
  clear_span();
  if (int c = int(lex->type) - int(b)) return c;
  return 0;
}

int Token::atom_cmp(const char& b) {
  clear_span();
  if (int c = lex->len() - 1) return c;
  if (int c = lex->span_a[0] - b) return c;
  return 0;
}

int Token::atom_cmp(const char* b) {
  clear_span();
  if (int c = strcmp_span(lex->span_a, lex->span_b, b)) return c;
  return 0;
}

int Token::atom_cmp(const Token* b) {
  clear_span();
  if (int c = lex->type - b->lex->type) return c;
  if (int c = lex->len() - b->lex->len()) return c;
  for (auto i = 0; i < lex->len(); i++) {
    if (auto c = lex->span_a[i] - b->lex->span_a[i]) return c;
  }
  return 0;
}

//----------------------------------------------------------------------------

const char* Token::unsafe_span_a() { return lex->span_a; }

ParseNode* Token::get_span() { return span; }

const ParseNode* Token::get_span() const { return span; }

void Token::set_span(ParseNode* n) {
  DCHECK(n);
  span = n;
}

void Token::clear_span() { span = nullptr; }

const char* Token::type_to_str() const { return lex->type_to_str(); }

uint32_t Token::type_to_color() const { return lex->type_to_color(); }

Token* Token::step_left() { return (this - 1)->get_span()->tok_a(); }

Token* Token::step_right() { return get_span()->tok_b() + 1; }

const char* Token::debug_span_a() const { return lex->span_a; }

const char* Token::debug_span_b() const { return lex->span_b; }

const Lexeme* Token::get_lex_debug() const { return lex; }

//----------------------------------------------------------------------------

void Token::dump_token() const {
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

//------------------------------------------------------------------------------
