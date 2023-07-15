// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

#include <stdio.h>
#include <string>
#include <vector>
#include <array>

#include "examples/c99_parser/c_constants.hpp"
#include "examples/c99_parser/SST.hpp"
#include "examples/c99_parser/Lexeme.hpp"
#include "examples/c99_parser/C99Lexer.hpp"
#include "examples/c99_parser/Token.hpp"
#include "examples/c99_parser/ParseNode.hpp"
#include "examples/c99_parser/TypeScope.hpp"

using namespace matcheroni;

struct C99Parser;
struct Lexeme;
struct ParseNode;
struct SlabAlloc;
struct Token;
struct TypeScope;

typedef Span<Token> tspan;

//------------------------------------------------------------------------------

class C99Parser {
 public:
  C99Parser();

  void reset();
  bool parse(std::vector<Lexeme>& lexemes);

  Token* match_builtin_type_base  (tspan s);
  Token* match_builtin_type_prefix(tspan s);
  Token* match_builtin_type_suffix(tspan s);

  Token* match_class_type  (tspan s);
  Token* match_struct_type (tspan s);
  Token* match_union_type  (tspan s);
  Token* match_enum_type   (tspan s);
  Token* match_typedef_type(tspan s);

  void add_class_type  (Token* a);
  void add_struct_type (Token* a);
  void add_union_type  (Token* a);
  void add_enum_type   (Token* a);
  void add_typedef_type(Token* a);

  void push_scope();
  void pop_scope();

  void append_node(ParseNode* node);
  void enclose_nodes(ParseNode* start, ParseNode* node);

  void dump_stats();
  void dump_lexemes();
  void dump_tokens();

  void parser_rewind(tspan s);

  //----------------------------------------

  std::vector<Token> tokens;
  TypeScope* type_scope;

  ParseNode* head = nullptr;
  ParseNode* tail = nullptr;
  ParseNode* root = nullptr;

  Token* global_cursor;
  inline static int rewind_count = 0;
  inline static int didnt_rewind = 0;
};

//------------------------------------------------------------------------------

template<>
inline int matcheroni::atom_cmp(void* ctx, Token* a, LexemeType b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

template<>
inline int matcheroni::atom_cmp(void* ctx, Token* a, char b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

template<>
inline int matcheroni::atom_cmp(void* ctx, Token* a, const char* b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

template<int N>
inline int atom_cmp(void* ctx, Token* a, const StringParam<N>& b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

template<>
inline int matcheroni::atom_cmp(void* ctx, Token* a, const Token* b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

template<>
inline void matcheroni::parser_rewind(void* ctx, tspan s) {
  ((C99Parser*)ctx)->parser_rewind(s);
}

//------------------------------------------------------------------------------
// Sorted string table matcher thing.

template<const auto& table>
struct SST;

template<typename T, auto N, const std::array<T, N>& table>
struct SST<table> {

  constexpr inline static int top_bit(int x) {
    for (int b = 31; b >= 0; b--) {
      if (x & (1 << b)) return (1 << b);
    }
    return 0;
  }

  constexpr static bool contains(const char* s) {
    for (auto t : table) {
      if (__builtin_strcmp(t, s) == 0) return true;
    }
    return false;
  }

  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    int bit = top_bit(N);
    int index = 0;

    // I'm not actually sure if 8 is the best tradeoff but it seems OK
    if (N > 8) {
      // Binary search for large tables
      while(1) {
        auto new_index = index | bit;
        if (new_index < N) {
          auto lit = table[new_index];
          auto c = atom_cmp(ctx, a, lit);
          if (c == 0) return a + 1;
          if (c > 0) index = new_index;
        }
        if (bit == 0) return nullptr;
        bit >>= 1;
      }
    }
    else {
      // Linear scan for small tables
      for (auto lit : table) {
        if (atom_cmp(ctx, a, lit) == 0) return a + 1;
      }
    }

    return nullptr;
  }
};

//------------------------------------------------------------------------------
