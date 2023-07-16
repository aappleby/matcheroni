// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include <stdint.h>
#include "examples/c99_parser/c_constants.hpp"
#include "examples/c99_parser/Lexeme.hpp"
#include "matcheroni/Matcheroni.hpp"

struct Lexeme;
struct C99ParseNode;

//------------------------------------------------------------------------------

struct Token {
  Token(const Lexeme* lex);

  void dump_token() const;

  const Lexeme* lex;
};

//------------------------------------------------------------------------------

template<>
inline int matcheroni::atom_cmp(void* ctx, const Token* a, LexemeType b) {
  if (int c = int(a->lex->type) - int(b)) return c;
  return 0;
}

template<>
inline int matcheroni::atom_cmp(void* ctx, const Token* a, char b) {
  if (int c = a->lex->len() - 1) return c;
  if (int c = a->lex->span.a[0] - b) return c;
  return 0;
}

template<>
inline int matcheroni::atom_cmp(void* ctx, const Token* a, const char* b) {
  if (int c = strcmp_span(a->lex->span, b)) return c;
  return 0;
}

template<>
inline int matcheroni::atom_cmp<Token, const Token*>(void* ctx, const Token* a, const Token* b) {
  if (int c = a->lex->type - b->lex->type) return c;
  if (int c = a->lex->len() - b->lex->len()) return c;
  for (auto i = 0; i < a->lex->len(); i++) {
    if (auto c = a->lex->span.a[i] - b->lex->span.a[i]) return c;
  }
  return 0;
}

template<int N>
inline int atom_cmp(void* ctx, const Token* a, const matcheroni::StringParam<N> b) {
  if (int c = a->lex->len() - b.str_len) return c;
  for (auto i = 0; i < b.str_len; i++) {
    if (auto c = a->lex->span.a[i] - b.str_val[i]) return c;
  }
  return 0;
}

//------------------------------------------------------------------------------
