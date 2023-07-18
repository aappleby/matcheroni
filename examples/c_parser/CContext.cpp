// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c_parser/CContext.hpp"

#include "examples/c_parser/c_parse_nodes.hpp"

using namespace matcheroni;

//------------------------------------------------------------------------------

CContext::CContext() {
  type_scope = new CScope();
  tokens.reserve(65536);
}

//------------------------------------------------------------------------------

void CContext::reset() {
  NodeContext::reset();

  tokens.clear();
  while (type_scope->parent) pop_scope();
  type_scope->clear();
}

//------------------------------------------------------------------------------

bool CContext::parse(std::vector<CToken>& lexemes) {

  for (auto& t : lexemes) {
    if (!t.is_gap()) {
      tokens.push_back(t);
    }
  }

  // Skip over BOF, stop before EOF
  auto tok_a = tokens.data() + 1;
  auto tok_b = tokens.data() + tokens.size() - 1;
  lex_span s(tok_a, tok_b);

  for (auto c = s.a; c < s.b; c++) {
    c->dump();
    printf("\n");
  }

  auto end = NodeTranslationUnit::match(*this, s);
  return end.is_valid();
}

//------------------------------------------------------------------------------

lex_span CContext::match_class_type(lex_span s) {
  return type_scope->has_class_type(*this, s) ? s.advance(1) : s.fail();
}

lex_span CContext::match_struct_type(lex_span s) {
  return type_scope->has_struct_type(*this, s) ? s.advance(1) : s.fail();
}

lex_span CContext::match_union_type(lex_span s) {
  return type_scope->has_union_type(*this, s) ? s.advance(1) : s.fail();
}

lex_span CContext::match_enum_type(lex_span s) {
  return type_scope->has_enum_type(*this, s) ? s.advance(1) : s.fail();
}

lex_span CContext::match_typedef_type(lex_span s) {
  return type_scope->has_typedef_type(*this, s) ? s.advance(1) : s.fail();
}

void CContext::add_class_type  (const CToken* a) { type_scope->add_class_type(*this, a); }
void CContext::add_struct_type (const CToken* a) { type_scope->add_struct_type(*this, a); }
void CContext::add_union_type  (const CToken* a) { type_scope->add_union_type(*this, a); }
void CContext::add_enum_type   (const CToken* a) { type_scope->add_enum_type(*this, a); }
void CContext::add_typedef_type(const CToken* a) { type_scope->add_typedef_type(*this, a); }

//----------------------------------------------------------------------------

void CContext::push_scope() {
  CScope* new_scope = new CScope();
  new_scope->parent = type_scope;
  type_scope = new_scope;
}

void CContext::pop_scope() {
  CScope* old_scope = type_scope->parent;
  if (old_scope) {
    delete type_scope;
    type_scope = old_scope;
  }
}

//----------------------------------------------------------------------------

lex_span CContext::match_builtin_type_base(lex_span s) {
  if (!s.is_valid() || s.is_empty()) return s.fail();
  if (SST<builtin_type_base>::match(s.a->a, s.a->b)) {
    return s.advance(1);
  }
  else {
    return s.fail();
  }
}

lex_span CContext::match_builtin_type_prefix(lex_span s) {
  if (!s.is_valid() || s.is_empty()) return s.fail();
  if (SST<builtin_type_prefix>::match(s.a->a, s.a->b)) {
    return s.advance(1);
  }
  else {
    return s.fail();
  }
}

lex_span CContext::match_builtin_type_suffix(lex_span s) {
  if (!s.is_valid() || s.is_empty()) return s.fail();
  if (SST<builtin_type_suffix>::match(s.a->a, s.a->b)) {
    return s.advance(1);
  }
  else {
    return s.fail();
  }
}

//------------------------------------------------------------------------------
