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

using TokenSpan = matcheroni::Span<CToken>;

//------------------------------------------------------------------------------

class CContext : public parseroni::NodeContext<CNode> {
 public:

  using AtomType = CToken;
  using SpanType = matcheroni::Span<CToken>;
  using NodeType = CNode;

  CContext();

  static int atom_cmp(char a, int b) {
    return (unsigned char)a - b;
  }

  static int atom_cmp(const CToken& a, const LexemeType& b) {
    return a.type - b;
  }

  static int atom_cmp(const CToken& a, const char& b) {
    if (auto d = a.text.len() - 1) return d;
    return a.text.begin[0] - b;
  }

  static int atom_cmp(const CToken& a, const matcheroni::TextSpan& b) {
    return strcmp_span(a.text, b);
  }

  void reset();
  //bool parse(std::vector<CToken>& lexemes);
  bool parse(matcheroni::TextSpan text, TokenSpan lexemes);

  TokenSpan match_builtin_type_base  (TokenSpan body);
  TokenSpan match_builtin_type_prefix(TokenSpan body);
  TokenSpan match_builtin_type_suffix(TokenSpan body);

  TokenSpan match_class_type  (TokenSpan body);
  TokenSpan match_struct_type (TokenSpan body);
  TokenSpan match_union_type  (TokenSpan body);
  TokenSpan match_enum_type   (TokenSpan body);
  TokenSpan match_typedef_type(TokenSpan body);

  void add_class_type  (const CToken* a);
  void add_struct_type (const CToken* a);
  void add_union_type  (const CToken* a);
  void add_enum_type   (const CToken* a);
  void add_typedef_type(const CToken* a);

  void push_scope();
  void pop_scope();

  void append_node(CNode* node);
  void enclose_nodes(CNode* start, CNode* node);

  void debug_dump(std::string& out) {
    for (auto node = top_head; node; node = node->node_next) {
      node->debug_dump(out);
    }
  }

  //----------------------------------------

  matcheroni::TextSpan text_span;
  TokenSpan  lexemes;

  std::vector<CToken> tokens;
  CScope* type_scope;
};

//------------------------------------------------------------------------------
