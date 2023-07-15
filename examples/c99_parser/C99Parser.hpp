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
