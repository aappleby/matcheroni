// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include <stdint.h>

#include "examples/c_parser/c_constants.hpp"
#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

//------------------------------------------------------------------------------

struct CToken {
  CToken(LexemeType type, matcheroni::TextSpan s);

  int len() const;
  bool is_bof() const;
  bool is_eof() const;
  bool is_gap() const;

  const char* type_to_str() const;
  uint32_t type_to_color() const;
  void dump_lexeme() const;

  //----------------------------------------

  LexemeType type;
  matcheroni::TextSpan span;
};

//------------------------------------------------------------------------------

template <>
inline int matcheroni::atom_cmp(void* ctx, const CToken& a, const LexemeType& b) {
  if (int c = int(a.type) - int(b)) return c;
  return 0;
}

template <>
inline int matcheroni::atom_cmp(void* ctx, const CToken& a, const char& b) {
  if (int c = a.len() - 1) return c;
  if (int c = a.span.a[0] - b) return c;
  return 0;
}

template<>
inline int matcheroni::atom_cmp(void* ctx, const CToken& a, const TextSpan& b) {
  return strcmp_span(a.span, b);
}

inline int atom_cmp(void* ctx, const CToken& a, const char*& b) {
  return strcmp_span(a.span, b);
}

template <>
inline int matcheroni::atom_cmp(void* ctx, const CToken& a, const CToken& b) {
  if (int c = a.type - b.type) return c;
  if (int c = a.len() - b.len()) return c;
  for (auto i = 0; i < a.len(); i++) {
    if (auto c = a.span.a[i] - b.span.a[i]) return c;
  }
  return 0;
}

template <int N>
inline int atom_cmp(void* ctx, const CToken& a,
                    const matcheroni::StringParam<N>& b) {
  if (int c = a.len() - b.str_len) return c;
  for (auto i = 0; i < b.str_len; i++) {
    if (auto c = a.span.a[i] - b.str_val[i]) return c;
  }
  return 0;
}

//------------------------------------------------------------------------------
