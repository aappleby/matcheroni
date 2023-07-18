// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include <vector>
#include "matcheroni/Matcheroni.hpp"

struct CToken;
struct CContext;
typedef matcheroni::Span<CToken> TokSpan;

//------------------------------------------------------------------------------

struct CScope {

  using token_list = std::vector<const CToken*>;

  void clear();
  bool has_type(CContext& ctx, TokSpan s, token_list& types);
  void add_type(CContext& ctx, const CToken* a, token_list& types);

  bool has_class_type  (CContext& ctx, TokSpan s);
  bool has_struct_type (CContext& ctx, TokSpan s);
  bool has_union_type  (CContext& ctx, TokSpan s);
  bool has_enum_type   (CContext& ctx, TokSpan s);
  bool has_typedef_type(CContext& ctx, TokSpan s);

  void add_class_type  (CContext& ctx, const CToken* a);
  void add_struct_type (CContext& ctx, const CToken* a);
  void add_union_type  (CContext& ctx, const CToken* a);
  void add_enum_type   (CContext& ctx, const CToken* a);
  void add_typedef_type(CContext& ctx, const CToken* a);

  CScope* parent;
  token_list class_types;
  token_list struct_types;
  token_list union_types;
  token_list enum_types;
  token_list typedef_types;
};

//------------------------------------------------------------------------------
