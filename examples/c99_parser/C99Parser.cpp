// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c99_parser/C99Parser.hpp"

//#include "examples/c99_parser/c99_parse_nodes.hpp"

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

void C99Parser::add_class_type  (Token* a) { type_scope->add_class_type(a); }
void C99Parser::add_struct_type (Token* a) { type_scope->add_struct_type(a); }
void C99Parser::add_union_type  (Token* a) { type_scope->add_union_type(a); }
void C99Parser::add_enum_type   (Token* a) { type_scope->add_enum_type(a); }
void C99Parser::add_typedef_type(Token* a) { type_scope->add_typedef_type(a); }

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
#if 0
void C99Parser::append_node(ParseNode* node) {
  if (tail) {
    tail->next = node;
    node->prev = tail;
    tail = node;
  } else {
    head = node;
    tail = node;
  }
}

void C99Parser::enclose_nodes(ParseNode* start, ParseNode* node) {
  // Is this right? Who knows. :D
  node->head = start;
  node->tail = tail;

  tail->next = node;
  start->prev = nullptr;

  tail = node;
}

//------------------------------------------------------------------------------

Token* C99Parser::match_builtin_type_base(Token* a, Token* b) {
  if (!a || a == b) return nullptr;
  return SST<builtin_type_base>::match(this, a, b);
}

Token* C99Parser::match_builtin_type_prefix(Token* a, Token* b) {
  if (!a || a == b) return nullptr;
  return SST<builtin_type_prefix>::match(this, a, b);
}

Token* C99Parser::match_builtin_type_suffix(Token* a, Token* b) {
  if (!a || a == b) return nullptr;
  return SST<builtin_type_suffix>::match(this, a, b);
}

//------------------------------------------------------------------------------

void C99Parser::dump_tokens() {
  for (auto& t : tokens) {
    t.dump_token();
  }
}

//------------------------------------------------------------------------------
#endif
