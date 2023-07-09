// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c99_parser/C99Parser.hpp"

void TypeScope::clear() {
  class_types.clear();
  struct_types.clear();
  union_types.clear();
  enum_types.clear();
  typedef_types.clear();
}

bool TypeScope::has_type(void* ctx, Token* a, Token* b, token_list& types) {
  if(atom_cmp(ctx, a, LEX_IDENTIFIER)) {
    return false;
  }
  /*+*/parser_rewind(ctx, a, b);

  for (const auto c : types) {
    if (atom_cmp(ctx, a, c) == 0) {
      return true;
    }
    else {
    }
  }

  return false;
}

void TypeScope::add_type(Token* a, token_list& types) {
  DCHECK(a->atom_cmp(LEX_IDENTIFIER) == 0);

  for (const auto& c : types) {
    if (a->atom_cmp(c) == 0) return;
  }

  types.push_back(a);
}

//----------------------------------------

bool TypeScope::has_class_type  (void* ctx, Token* a, Token* b) { if (has_type(ctx, a, b, class_types  )) return true; if (parent) return parent->has_class_type  (ctx, a, b); else return false; }
bool TypeScope::has_struct_type (void* ctx, Token* a, Token* b) { if (has_type(ctx, a, b, struct_types )) return true; if (parent) return parent->has_struct_type (ctx, a, b); else return false; }
bool TypeScope::has_union_type  (void* ctx, Token* a, Token* b) { if (has_type(ctx, a, b, union_types  )) return true; if (parent) return parent->has_union_type  (ctx, a, b); else return false; }
bool TypeScope::has_enum_type   (void* ctx, Token* a, Token* b) { if (has_type(ctx, a, b, enum_types   )) return true; if (parent) return parent->has_enum_type   (ctx, a, b); else return false; }
bool TypeScope::has_typedef_type(void* ctx, Token* a, Token* b) { if (has_type(ctx, a, b, typedef_types)) return true; if (parent) return parent->has_typedef_type(ctx, a, b); else return false; }

void TypeScope::add_class_type  (Token* a) { return add_type(a, class_types  ); }
void TypeScope::add_struct_type (Token* a) { return add_type(a, struct_types ); }
void TypeScope::add_union_type  (Token* a) { return add_type(a, union_types  ); }
void TypeScope::add_enum_type   (Token* a) { return add_type(a, enum_types   ); }
void TypeScope::add_typedef_type(Token* a) { return add_type(a, typedef_types); }
