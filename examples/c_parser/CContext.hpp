// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

#include <array>
#include <stdio.h>
#include <string>
#include <vector>

#include "examples/c_parser/c_constants.hpp"
#include "examples/c_lexer/CToken.hpp"
#include "examples/c_lexer/CLexer.hpp"
#include "examples/c_parser/CNode.hpp"
#include "examples/c_parser/CScope.hpp"
#include "examples/SST.hpp"

struct CToken;
struct CNode;
struct CContext;
struct CScope;

using lex_span = matcheroni::Span<CToken>;

//------------------------------------------------------------------------------

class CContext : public matcheroni::ContextBase<CNode> {
 public:
  CContext();

  void reset();
  bool parse(std::vector<CToken>& tokens);

  lex_span match_builtin_type_base  (lex_span s);
  lex_span match_builtin_type_prefix(lex_span s);
  lex_span match_builtin_type_suffix(lex_span s);

  lex_span match_class_type  (lex_span s);
  lex_span match_struct_type (lex_span s);
  lex_span match_union_type  (lex_span s);
  lex_span match_enum_type   (lex_span s);
  lex_span match_typedef_type(lex_span s);

  void add_class_type  (const CToken* a);
  void add_struct_type (const CToken* a);
  void add_union_type  (const CToken* a);
  void add_enum_type   (const CToken* a);
  void add_typedef_type(const CToken* a);

  void push_scope();
  void pop_scope();

  void append_node(CNode* node);
  void enclose_nodes(CNode* start, CNode* node);

  void dump_stats();
  void dump_lexemes();
  //void dump_tokens();

  void parser_rewind(lex_span s);

  //----------------------------------------

  std::vector<CToken> tokens;
  CScope* type_scope;
};

//------------------------------------------------------------------------------
