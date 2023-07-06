// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include <stdint.h>

#include "c99/Lexeme.hpp"
#include "Matcheroni.hpp"

using namespace matcheroni;

struct ParseNode;

//------------------------------------------------------------------------------
// Tokens associate lexemes with parse nodes.
// Tokens store bookkeeping data during parsing.

struct Token {
  Token(const Lexeme* lex);

  //----------------------------------------
  // These methods REMOVE THE SPAN FROM THE NODE - that's why they're not
  // const. This is required to ensure that if an Opt<> matcher fails to match
  // a branch, when it tries to match the next branch we will always pull the
  // defunct nodes off the tokens.

  int atom_cmp(const LexemeType& b);
  int atom_cmp(const char& b);
  int atom_cmp(const char* b);

  template <int N>
  int atom_cmp(const StringParam<N>& b) {
    clear_span();
    if (int c = lex->len() - b.str_len) return c;
    for (auto i = 0; i < b.str_len; i++) {
      if (auto c = lex->span_a[i] - b.str_val[i]) return c;
    }
    return 0;
  }

  int atom_cmp(const Token* b);
  const char* unsafe_span_a();

  //----------------------------------------

  ParseNode* get_span();
  const ParseNode* get_span() const;
  void set_span(ParseNode* n);
  void clear_span();
  const char* type_to_str() const;
  uint32_t type_to_color() const;
  void dump_token() const;

  Token* step_left();
  Token* step_right();

  //----------------------------------------

  const char* debug_span_a() const;
  const char* debug_span_b() const;
  const Lexeme* get_lex_debug() const;

  bool dirty = false;

private:
  ParseNode* span;
  const Lexeme* lex;
};

//------------------------------------------------------------------------------

int atom_cmp(void* ctx, Token* a, LexemeType b);
int atom_cmp(void* ctx, Token* a, char b);
int atom_cmp(void* ctx, Token* a, const char* b);
int atom_cmp(void* ctx, Token* a, const Token* b);

//template<int N>
//int atom_cmp(void* ctx, Token* a, const StringParam<N>& b);

void atom_rewind(void* ctx, Token* a, Token* b);

//------------------------------------------------------------------------------
