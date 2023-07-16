// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c_parser/CParser.hpp"

#include "examples/c_parser/c_parse_nodes.hpp"

using namespace matcheroni;

//------------------------------------------------------------------------------

CParser::CParser() {
  type_scope = new CScope();
  tokens.reserve(65536);
}

//------------------------------------------------------------------------------

void CParser::reset() {
  ContextBase::reset();

  tokens.clear();
  while (type_scope->parent) pop_scope();
  type_scope->clear();
}

//------------------------------------------------------------------------------

#if 0
bool CParser::parse(std::vector<CLexeme>& lexemes) {

  for (auto i = 0; i < lexemes.size(); i++) {
    auto l = &lexemes[i];
    if (!l->is_gap()) {
      tokens.push_back(Token(l));
    }
  }

  // Skip over BOF, stop before EOF
  auto tok_a = tokens.data() + 1;
  auto tok_b = tokens.data() + tokens.size() - 1;
  lex_span s(tok_a, tok_b);

  auto end = NodeTranslationUnit::match(this, s);
  return end.is_valid();
}
#endif

//------------------------------------------------------------------------------

lex_span CParser::match_class_type(lex_span s) {
  return type_scope->has_class_type(this, s) ? s.advance(1) : s.fail();
}

lex_span CParser::match_struct_type(lex_span s) {
  return type_scope->has_struct_type(this, s) ? s.advance(1) : s.fail();
}

lex_span CParser::match_union_type(lex_span s) {
  return type_scope->has_union_type(this, s) ? s.advance(1) : s.fail();
}

lex_span CParser::match_enum_type(lex_span s) {
  return type_scope->has_enum_type(this, s) ? s.advance(1) : s.fail();
}

lex_span CParser::match_typedef_type(lex_span s) {
  return type_scope->has_typedef_type(this, s) ? s.advance(1) : s.fail();
}

void CParser::add_class_type  (const CToken* a) { type_scope->add_class_type(a); }
void CParser::add_struct_type (const CToken* a) { type_scope->add_struct_type(a); }
void CParser::add_union_type  (const CToken* a) { type_scope->add_union_type(a); }
void CParser::add_enum_type   (const CToken* a) { type_scope->add_enum_type(a); }
void CParser::add_typedef_type(const CToken* a) { type_scope->add_typedef_type(a); }

//----------------------------------------------------------------------------

void CParser::push_scope() {
  CScope* new_scope = new CScope();
  new_scope->parent = type_scope;
  type_scope = new_scope;
}

void CParser::pop_scope() {
  CScope* old_scope = type_scope->parent;
  if (old_scope) {
    delete type_scope;
    type_scope = old_scope;
  }
}

//----------------------------------------------------------------------------

lex_span CParser::match_builtin_type_base(lex_span s) {
  if (!s.is_valid() || s.is_empty()) return s.fail();
  if (SST<builtin_type_base>::match(s.a->lex->span.a, s.a->lex->span.b)) {
    return s.advance(1);
  }
  else {
    return s.fail();
  }
}

lex_span CParser::match_builtin_type_prefix(lex_span s) {
  if (!s.is_valid() || s.is_empty()) return s.fail();
  if (SST<builtin_type_prefix>::match(s.a->lex->span.a, s.a->lex->span.b)) {
    return s.advance(1);
  }
  else {
    return s.fail();
  }
}

lex_span CParser::match_builtin_type_suffix(lex_span s) {
  if (!s.is_valid() || s.is_empty()) return s.fail();
  if (SST<builtin_type_suffix>::match(s.a->lex->span.a, s.a->lex->span.b)) {
    return s.advance(1);
  }
  else {
    return s.fail();
  }
}

//------------------------------------------------------------------------------

#if 0
void CParser::dump_tokens() {
  for (auto& t : tokens) {
    t.dump_token();
  }
}
#endif

//------------------------------------------------------------------------------
