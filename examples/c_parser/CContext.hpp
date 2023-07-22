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

using TokSpan = matcheroni::Span<CToken>;

//------------------------------------------------------------------------------

class CContext : public matcheroni::NodeContext<TokSpan, CNode> {
 public:
  CContext();

  static int atom_cmp(char a, int b) {
    return a - b;
  }

  static int atom_cmp(const CToken& a, const LexemeType& b) {
    return a.type - b;
  }

  static int atom_cmp(const CToken& a, const char& b) {
    if (auto d = a.text.len() - 1) return d;
    return a.text.a[0] - b;
  }

  static int atom_cmp(const CToken& a, const matcheroni::TextSpan& b) {
    return strcmp_span(a.text, b);
  }

  CNode* top_head() { return (CNode*)_top_head; }
  CNode* top_tail() { return (CNode*)_top_tail; }

  const CNode* top_head() const { return (CNode*)_top_head; }
  const CNode* top_tail() const { return (CNode*)_top_tail; }

  void set_head(CNode* head) { _top_head = head; }
  void set_tail(CNode* tail) { _top_tail = tail; }

  void reset();
  //bool parse(std::vector<CToken>& lexemes);
  bool parse(matcheroni::TextSpan text, TokSpan lexemes);

  TokSpan match_builtin_type_base  (TokSpan body);
  TokSpan match_builtin_type_prefix(TokSpan body);
  TokSpan match_builtin_type_suffix(TokSpan body);

  TokSpan match_class_type  (TokSpan body);
  TokSpan match_struct_type (TokSpan body);
  TokSpan match_union_type  (TokSpan body);
  TokSpan match_enum_type   (TokSpan body);
  TokSpan match_typedef_type(TokSpan body);

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
    for (auto node = top_head(); node; node = node->node_next()) {
      node->debug_dump(out);
    }
  }

  //----------------------------------------

  matcheroni::TextSpan text_span;
  TokSpan  lexemes;

  std::vector<CToken> tokens;
  CScope* type_scope;
};

//------------------------------------------------------------------------------
