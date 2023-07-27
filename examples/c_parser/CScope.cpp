// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c_parser/CScope.hpp"

#include "examples/c_parser/c_constants.hpp"
#include "examples/c_parser/CContext.hpp"
#include "examples/c_lexer/CToken.hpp"

#include <string_view>

using matcheroni::TextSpan;

using std::string_view;

void CScope::clear() {
  class_types.clear();
  struct_types.clear();
  union_types.clear();
  enum_types.clear();
  typedef_types.clear();
}

bool CScope::has_type(CContext& ctx, TokenSpan body, token_list& types) {
  if(ctx.atom_cmp(*body.begin, LEX_IDENTIFIER)) {
    return false;
  }
  /*+*/ctx.rewind(body);

  TextSpan span(body.begin->text.begin, body.begin->text.end);

  for (const auto& c : types) {
    if (strcmp_span(span, c) == 0) return true;
  }

  return false;
}

void CScope::add_type(CContext& ctx, const CToken* a, token_list& types) {
  matcheroni_assert(ctx.atom_cmp(*a, LEX_IDENTIFIER) == 0);

  TextSpan span(a->text.begin, a->text.end);

  for (const auto& c : types) {
    if (strcmp_span(span, c) == 0) return;
  }

  types.push_back(span);
}

//----------------------------------------

void CScope::add_typedef_type(const char* t) {
  TextSpan span = matcheroni::utils::to_span(t);

  for (const auto& c : typedef_types) {
    if (strcmp_span(span, c) == 0) return;
  }

  typedef_types.push_back(span);
}

//----------------------------------------

bool CScope::has_class_type  (CContext& ctx, TokenSpan body) { if (has_type(ctx, body, class_types  )) return true; if (parent) return parent->has_class_type  (ctx, body); else return false; }
bool CScope::has_struct_type (CContext& ctx, TokenSpan body) { if (has_type(ctx, body, struct_types )) return true; if (parent) return parent->has_struct_type (ctx, body); else return false; }
bool CScope::has_union_type  (CContext& ctx, TokenSpan body) { if (has_type(ctx, body, union_types  )) return true; if (parent) return parent->has_union_type  (ctx, body); else return false; }
bool CScope::has_enum_type   (CContext& ctx, TokenSpan body) { if (has_type(ctx, body, enum_types   )) return true; if (parent) return parent->has_enum_type   (ctx, body); else return false; }
bool CScope::has_typedef_type(CContext& ctx, TokenSpan body) { if (has_type(ctx, body, typedef_types)) return true; if (parent) return parent->has_typedef_type(ctx, body); else return false; }

void CScope::add_class_type  (CContext& ctx, const CToken* a) { return add_type(ctx, a, class_types  ); }
void CScope::add_struct_type (CContext& ctx, const CToken* a) { return add_type(ctx, a, struct_types ); }
void CScope::add_union_type  (CContext& ctx, const CToken* a) { return add_type(ctx, a, union_types  ); }
void CScope::add_enum_type   (CContext& ctx, const CToken* a) { return add_type(ctx, a, enum_types   ); }
void CScope::add_typedef_type(CContext& ctx, const CToken* a) { return add_type(ctx, a, typedef_types); }
