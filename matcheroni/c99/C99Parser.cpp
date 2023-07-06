// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "c99/C99Parser.hpp"

#include "utils.hpp"

#include "c99/C99Lexer.hpp"
#include "c99/ParseNode.hpp"
#include "c99/SlabAlloc.hpp"
#include "c99/TypeScope.hpp"
#include "c99/c_parse_nodes.hpp"

using namespace matcheroni;

//------------------------------------------------------------------------------

C99Parser::C99Parser() {
  type_scope = new TypeScope();
  tokens.reserve(65536);
}

//------------------------------------------------------------------------------

void C99Parser::reset() {

  tokens.clear();
  ParseNode::slabs.reset();

  while (type_scope->parent) pop_scope();
  type_scope->clear();
}

//------------------------------------------------------------------------------

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

  global_cursor = tok_a;
  auto end = NodeTranslationUnit::match(this, tok_a, tok_b);

  root = tok_a->get_span();

#ifdef DEBUG
  if (root) {
    root->check_sanity();
  }
#endif

  if (!end || end != tok_b) return false;

  return true;
}

//------------------------------------------------------------------------------

int C99Parser::atom_cmp(Token* a, const LexemeType& b) {
  DCHECK(a == global_cursor);
  auto result = a->atom_cmp(b);
  if (result == 0) global_cursor++;
  return result;
}

int C99Parser::atom_cmp(Token* a, const char& b) {
  DCHECK(a == global_cursor);
  auto result = a->atom_cmp(b);
  if (result == 0) global_cursor++;
  return result;
}

int C99Parser::atom_cmp(Token* a, const char* b) {
  DCHECK(a == global_cursor);
  auto result = a->atom_cmp(b);
  if (result == 0) global_cursor++;
  return result;
}

/*
template<int N>
int C99Parser::atom_cmp(Token* a, const StringParam<N>& b) {
  DCHECK(a == global_cursor);
  auto result = a->atom_cmp(b);
  if (result == 0) global_cursor++;
  return result;
}
*/

int C99Parser::atom_cmp(Token* a, const Token* b) {
  DCHECK(a == global_cursor);
  auto result = a->atom_cmp(b);
  if (result == 0) global_cursor++;
  return result;
}

void C99Parser::atom_rewind(Token* a, Token* b) {
  // printf("rewind to %20.20s\n", a->debug_span_a());

  /*
  if (a < global_cursor) {
    static constexpr int context_len = 60;
    printf("[");
    print_escaped(global_cursor->get_lex_debug()->span_a, context_len,
  0x804080); printf("]\n"); printf("[");
    print_escaped(a->get_lex_debug()->span_a, context_len, 0x804040);
    printf("]\n");
  }
  */

  DCHECK(a <= global_cursor);

  if (a < global_cursor) {
    rewind_count++;
  } else {
    didnt_rewind++;
  }

  global_cursor = a;
}

//------------------------------------------------------------------------------

int atom_cmp(void* ctx, Token* a, LexemeType b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

int atom_cmp(void* ctx, Token* a, char b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

int atom_cmp(void* ctx, Token* a, const char* b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

/*
template<int N>
inline int atom_cmp(void* ctx, Token* a, const StringParam<N>& b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}
*/

int atom_cmp(void* ctx, Token* a, const Token* b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

void atom_rewind(void* ctx, Token* a, Token* b) {
  ((C99Parser*)ctx)->atom_rewind(a, b);
}

//------------------------------------------------------------------------------

Token* C99Parser::match_class_type(Token* a, Token* b) {
  return type_scope->has_class_type(this, a, b) ? a + 1 : nullptr;
}
Token* C99Parser::match_struct_type(Token* a, Token* b) {
  return type_scope->has_struct_type(this, a, b) ? a + 1 : nullptr;
}
Token* C99Parser::match_union_type(Token* a, Token* b) {
  return type_scope->has_union_type(this, a, b) ? a + 1 : nullptr;
}
Token* C99Parser::match_enum_type(Token* a, Token* b) {
  return type_scope->has_enum_type(this, a, b) ? a + 1 : nullptr;
}
Token* C99Parser::match_typedef_type(Token* a, Token* b) {
  return type_scope->has_typedef_type(this, a, b) ? a + 1 : nullptr;
}

void C99Parser::add_class_type(Token* a) { type_scope->add_class_type(a); }
void C99Parser::add_struct_type(Token* a) { type_scope->add_struct_type(a); }
void C99Parser::add_union_type(Token* a) { type_scope->add_union_type(a); }
void C99Parser::add_enum_type(Token* a) { type_scope->add_enum_type(a); }
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
