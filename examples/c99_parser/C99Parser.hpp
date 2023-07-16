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

struct C99Parser;
struct Lexeme;
struct C99ParseNode;
struct SlabAlloc;
struct Token;
struct TypeScope;

using tspan = matcheroni::Span<Token>;

//------------------------------------------------------------------------------

class C99Parser : public matcheroni::ContextBase<C99ParseNode> {
 public:
  C99Parser();

  void reset();
  bool parse(std::vector<Lexeme>& lexemes);

  tspan match_builtin_type_base  (tspan s);
  tspan match_builtin_type_prefix(tspan s);
  tspan match_builtin_type_suffix(tspan s);

  tspan match_class_type  (tspan s);
  tspan match_struct_type (tspan s);
  tspan match_union_type  (tspan s);
  tspan match_enum_type   (tspan s);
  tspan match_typedef_type(tspan s);

  void add_class_type  (const Token* a);
  void add_struct_type (const Token* a);
  void add_union_type  (const Token* a);
  void add_enum_type   (const Token* a);
  void add_typedef_type(const Token* a);

  void push_scope();
  void pop_scope();

  void append_node(C99ParseNode* node);
  void enclose_nodes(C99ParseNode* start, C99ParseNode* node);

  void dump_stats();
  void dump_lexemes();
  void dump_tokens();

  void parser_rewind(tspan s);

  //----------------------------------------

  std::vector<Token> tokens;
  TypeScope* type_scope;
};

//------------------------------------------------------------------------------
