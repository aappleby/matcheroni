// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include <vector>

struct Token;

//------------------------------------------------------------------------------

struct TypeScope {

  using token_list = std::vector<const Token*>;

  void clear();
  bool has_type(void* ctx, Token* a, Token* b, token_list& types);
  void add_type(Token* a, token_list& types);

  bool has_class_type  (void* ctx, Token* a, Token* b);
  bool has_struct_type (void* ctx, Token* a, Token* b);
  bool has_union_type  (void* ctx, Token* a, Token* b);
  bool has_enum_type   (void* ctx, Token* a, Token* b);
  bool has_typedef_type(void* ctx, Token* a, Token* b);

  void add_class_type  (Token* a);
  void add_struct_type (Token* a);
  void add_union_type  (Token* a);
  void add_enum_type   (Token* a);
  void add_typedef_type(Token* a);

  TypeScope* parent;
  token_list class_types;
  token_list struct_types;
  token_list union_types;
  token_list enum_types;
  token_list typedef_types;
};

//------------------------------------------------------------------------------
