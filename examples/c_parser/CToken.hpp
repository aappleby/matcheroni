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
  const CLexeme* lex;
};

//------------------------------------------------------------------------------

template<>
inline int matcheroni::atom_cmp(void* ctx, const CToken& a, const LexemeType& b) {
  return atom_cmp(ctx, *a.lex, b);
}

template<>
inline int matcheroni::atom_cmp(void* ctx, const CToken& a, const char& b) {
  return atom_cmp(ctx, *a.lex, b);
}

template<>
inline int matcheroni::atom_cmp(void* ctx, const CToken& a, const TextSpan& b) {
  return atom_cmp(ctx, *a.lex, b);
}

inline int atom_cmp(void* ctx, const CToken& a, const char*& b) {
  return atom_cmp(ctx, *a.lex, b);
}

template<>
inline int matcheroni::atom_cmp(void* ctx, const CToken& a, const CToken& b) {
  return atom_cmp(ctx, *a.lex, *b.lex);
}

template<int N>
inline int atom_cmp(void* ctx, const CToken& a, const matcheroni::StringParam<N>& b) {
  return atom_cmp(ctx, *a.lex, b);
}

//------------------------------------------------------------------------------
