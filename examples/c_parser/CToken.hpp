// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include <stdint.h>
#include "examples/c_parser/c_constants.hpp"
#include "examples/c_parser/CLexeme.hpp"
#include "matcheroni/Matcheroni.hpp"

struct CLexeme;
struct CNode;

//------------------------------------------------------------------------------

struct CToken {
  CToken(const CLexeme* lex);

  void dump_token() const;

  const CLexeme* lex;
};

//------------------------------------------------------------------------------

template<>
inline int matcheroni::atom_cmp(void* ctx, const CToken* a, LexemeType b) {
  if (int c = int(a->lex->type) - int(b)) return c;
  return 0;
}

template<>
inline int matcheroni::atom_cmp(void* ctx, const CToken* a, char b) {
  if (int c = a->lex->len() - 1) return c;
  if (int c = a->lex->span.a[0] - b) return c;
  return 0;
}

template<>
inline int matcheroni::atom_cmp(void* ctx, const CToken* a, const char* b) {
  if (int c = strcmp_span(a->lex->span, b)) return c;
  return 0;
}

template<>
inline int matcheroni::atom_cmp<CToken, const CToken*>(void* ctx, const CToken* a, const CToken* b) {
  if (int c = a->lex->type - b->lex->type) return c;
  if (int c = a->lex->len() - b->lex->len()) return c;
  for (auto i = 0; i < a->lex->len(); i++) {
    if (auto c = a->lex->span.a[i] - b->lex->span.a[i]) return c;
  }
  return 0;
}

template<int N>
inline int atom_cmp(void* ctx, const CToken* a, const matcheroni::StringParam<N> b) {
  if (int c = a->lex->len() - b.str_len) return c;
  for (auto i = 0; i < b.str_len; i++) {
    if (auto c = a->lex->span.a[i] - b.str_val[i]) return c;
  }
  return 0;
}

//------------------------------------------------------------------------------
