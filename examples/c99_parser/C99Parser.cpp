// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c99_parser/C99Parser.hpp"

#include "examples/c99_parser/c99_parse_nodes.hpp"

using namespace matcheroni;

//------------------------------------------------------------------------------

C99Parser::C99Parser() {
  type_scope = new TypeScope();
  tokens.reserve(65536);
}

//------------------------------------------------------------------------------

void C99Parser::reset() {
  Context::reset();

  tokens.clear();
  while (type_scope->parent) pop_scope();
  type_scope->clear();
}

//------------------------------------------------------------------------------

#if 0
bool C99Parser::parse(std::vector<Lexeme>& lexemes) {

  for (auto i = 0; i < lexemes.size(); i++) {
    auto l = &lexemes[i];
    if (!l->is_gap()) {
      tokens.push_back(Token(l));
    }
  }

  // Skip over BOF, stop before EOF
  auto tok_a = tokens.data() + 1;
  auto tok_b = tokens.data() + tokens.size() - 1;
  tspan s(tok_a, tok_b);

  auto end = NodeTranslationUnit::match(this, s);
  return end.is_valid();
}
#endif

//------------------------------------------------------------------------------

tspan C99Parser::match_class_type(tspan s) {
  return type_scope->has_class_type(this, s) ? s.advance(1) : s.fail();
}

tspan C99Parser::match_struct_type(tspan s) {
  return type_scope->has_struct_type(this, s) ? s.advance(1) : s.fail();
}

tspan C99Parser::match_union_type(tspan s) {
  return type_scope->has_union_type(this, s) ? s.advance(1) : s.fail();
}

tspan C99Parser::match_enum_type(tspan s) {
  return type_scope->has_enum_type(this, s) ? s.advance(1) : s.fail();
}

tspan C99Parser::match_typedef_type(tspan s) {
  return type_scope->has_typedef_type(this, s) ? s.advance(1) : s.fail();
}

void C99Parser::add_class_type  (const Token* a) { type_scope->add_class_type(a); }
void C99Parser::add_struct_type (const Token* a) { type_scope->add_struct_type(a); }
void C99Parser::add_union_type  (const Token* a) { type_scope->add_union_type(a); }
void C99Parser::add_enum_type   (const Token* a) { type_scope->add_enum_type(a); }
void C99Parser::add_typedef_type(const Token* a) { type_scope->add_typedef_type(a); }

//----------------------------------------------------------------------------

void C99Parser::push_scope() {
  TypeScope* new_scope = new TypeScope();
  new_scope->parent = type_scope;
  type_scope = new_scope;
}

void C99Parser::pop_scope() {
  TypeScope* old_scope = type_scope->parent;
  if (old_scope) {
    delete type_scope;
    type_scope = old_scope;
  }
}

//----------------------------------------------------------------------------

tspan C99Parser::match_builtin_type_base(tspan s) {
  if (!s.is_valid() || s.is_empty()) return s.fail();
  if (SST<builtin_type_base>::match(s.a->lex->span.a, s.a->lex->span.b)) {
    return s.advance(1);
  }
  else {
    return s.fail();
  }
}

tspan C99Parser::match_builtin_type_prefix(tspan s) {
  if (!s.is_valid() || s.is_empty()) return s.fail();
  if (SST<builtin_type_prefix>::match(s.a->lex->span.a, s.a->lex->span.b)) {
    return s.advance(1);
  }
  else {
    return s.fail();
  }
}

tspan C99Parser::match_builtin_type_suffix(tspan s) {
  if (!s.is_valid() || s.is_empty()) return s.fail();
  if (SST<builtin_type_suffix>::match(s.a->lex->span.a, s.a->lex->span.b)) {
    return s.advance(1);
  }
  else {
    return s.fail();
  }
}

//------------------------------------------------------------------------------

void C99Parser::dump_tokens() {
  for (auto& t : tokens) {
    t.dump_token();
  }
}

//------------------------------------------------------------------------------
