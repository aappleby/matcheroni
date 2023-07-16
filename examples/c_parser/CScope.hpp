// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include <vector>
#include "matcheroni/Matcheroni.hpp"

struct CToken;
typedef matcheroni::Span<CToken> lex_span;

//------------------------------------------------------------------------------

struct CScope {

  using token_list = std::vector<const CToken*>;

  void clear();
  bool has_type(void* ctx, lex_span s, token_list& types);
  void add_type(const CToken* a, token_list& types);

  bool has_class_type  (void* ctx, lex_span s);
  bool has_struct_type (void* ctx, lex_span s);
  bool has_union_type  (void* ctx, lex_span s);
  bool has_enum_type   (void* ctx, lex_span s);
  bool has_typedef_type(void* ctx, lex_span s);

  void add_class_type  (const CToken* a);
  void add_struct_type (const CToken* a);
  void add_union_type  (const CToken* a);
  void add_enum_type   (const CToken* a);
  void add_typedef_type(const CToken* a);

  CScope* parent;
  token_list class_types;
  token_list struct_types;
  token_list union_types;
  token_list enum_types;
  token_list typedef_types;
};

//------------------------------------------------------------------------------
