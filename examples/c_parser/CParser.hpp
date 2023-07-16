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
#include "examples/c_parser/CLexeme.hpp"
#include "examples/c_parser/CLexer.hpp"
#include "examples/c_parser/CNode.hpp"
#include "examples/c_parser/CScope.hpp"
#include "examples/c_parser/CToken.hpp"
#include "examples/SST.hpp"

struct CLexeme;
struct CNode;
struct CParser;
struct CScope;
struct CToken;

using tspan = matcheroni::Span<CToken>;

//------------------------------------------------------------------------------

class CParser : public matcheroni::ContextBase<CNode> {
 public:
  CParser();

  void reset();
  bool parse(std::vector<CLexeme>& lexemes);

  tspan match_builtin_type_base  (tspan s);
  tspan match_builtin_type_prefix(tspan s);
  tspan match_builtin_type_suffix(tspan s);

  tspan match_class_type  (tspan s);
  tspan match_struct_type (tspan s);
  tspan match_union_type  (tspan s);
  tspan match_enum_type   (tspan s);
  tspan match_typedef_type(tspan s);

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
  void dump_tokens();

  void parser_rewind(tspan s);

  //----------------------------------------

  std::vector<CToken> tokens;
  CScope* type_scope;
};

//------------------------------------------------------------------------------
