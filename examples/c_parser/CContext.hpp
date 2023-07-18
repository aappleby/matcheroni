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

class CContext : public matcheroni::NodeContext<CToken> {
 public:
  CContext();

  static bool atom_eq(char a, char b) {
    return a == b;
  }

  static bool atom_eq(const CToken& a, const LexemeType& b) {
    return a.type == b;
  }

  static bool atom_eq(const CToken& a, const char& b) {
    if (a.len() != 1) return false;
    return a.a[0] == b;
  }

  static bool atom_eq(const CToken& a, const matcheroni::TextSpan& b) {
    return strcmp_span(a, b) == 0;
  }

  /*
  int compare(const char& a, const char& b) {
    return int(a - b);
  }

  int compare(const CToken& a, const LexemeType& b) {
    if (int c = int(a.type) - int(b)) return c;
    return 0;
  }

  int compare(const CToken& a, const char& b) {
    if (int c = a.len() - 1) return c;
    if (int c = a.a[0] - b) return c;
    return 0;
  }

  int compare(const CToken& a, const matcheroni::TextSpan& b) {
    return strcmp_span(a, b);
  }
  */

#if 0
template <>
inline int matcheroni::atom_cmp(void* ctx, const CToken& a, const LexemeType& b) {
  if (int c = int(a.type) - int(b)) return c;
  return 0;
}

template <>
inline int matcheroni::atom_cmp(void* ctx, const CToken& a, const char& b) {
  if (int c = a.len() - 1) return c;
  if (int c = a.a[0] - b) return c;
  return 0;
}

template<>
inline int matcheroni::atom_cmp(void* ctx, const CToken& a, const TextSpan& b) {
  return strcmp_span(a, b);
}
#endif


  CNode* top_head() { return (CNode*)_top_head; }
  CNode* top_tail() { return (CNode*)_top_tail; }

  const CNode* top_head() const { return (CNode*)_top_head; }
  const CNode* top_tail() const { return (CNode*)_top_tail; }

  void set_head(CNode* head) { _top_head = head; }
  void set_tail(CNode* tail) { _top_tail = tail; }

  void reset();
  bool parse(std::vector<CToken>& lexemes);

  TokSpan match_builtin_type_base  (TokSpan s);
  TokSpan match_builtin_type_prefix(TokSpan s);
  TokSpan match_builtin_type_suffix(TokSpan s);

  TokSpan match_class_type  (TokSpan s);
  TokSpan match_struct_type (TokSpan s);
  TokSpan match_union_type  (TokSpan s);
  TokSpan match_enum_type   (TokSpan s);
  TokSpan match_typedef_type(TokSpan s);

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

  //----------------------------------------

  std::vector<CToken> tokens;
  CScope* type_scope;
};

//------------------------------------------------------------------------------
