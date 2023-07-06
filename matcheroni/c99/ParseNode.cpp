// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "c99/ParseNode.hpp"

#include "c99/Lexeme.hpp"     // for Lexeme
#include "c99/SlabAlloc.hpp"
#include "c99/Token.hpp"
#include "utils.hpp"
#include <stdio.h>        // for printf, putc, sscanf, stdout
#include <string>         // for string, allocator

//------------------------------------------------------------------------------

SlabAlloc ParseNode::slabs;

void* ParseNode::operator new(std::size_t size)   { return slabs.bump(size); }
void* ParseNode::operator new[](std::size_t size) { return slabs.bump(size); }
void  ParseNode::operator delete(void*)           { }
void  ParseNode::operator delete[](void*)         { }

//------------------------------------------------------------------------------

ParseNode::~ParseNode() {}

//------------------------------------------------------------------------------

bool ParseNode::in_range(const Token* a, const Token* b) {
  return _tok_a >= a && _tok_b <= b;
}

//------------------------------------------------------------------------------

void ParseNode::init_node(void* ctx, Token* tok_a, Token* tok_b, ParseNode* node_a, ParseNode* node_b) {
  constructor_count++;
  DCHECK(tok_a <= tok_b);

  this->_tok_a = tok_a;
  this->_tok_b = tok_b;

  // Attach all the tops under this node to it.
  //bool has_child = false;
  auto cursor = tok_a;
  while (cursor <= tok_b) {
    auto child = cursor->get_span();
    if (child) {
      //has_child = true;
      attach_child(child);
      cursor = child->tok_b() + 1;
    }
    else {
      cursor++;
    }
  }
  //assert(has_child);

  _tok_a->set_span(this);
  _tok_b->set_span(this);
}

//------------------------------------------------------------------------------

void ParseNode::init_leaf(void* ctx, Token* tok_a, Token* tok_b) {
  constructor_count++;
  DCHECK(tok_a <= tok_b);

  this->_tok_a = tok_a;
  this->_tok_b = tok_b;

  // Attach all the tops under this node to it.
  auto cursor = tok_a;
  while (cursor <= tok_b) {
    DCHECK(!cursor->get_span());
    cursor++;
  }

  _tok_a->set_span(this);
  _tok_b->set_span(this);
}

//------------------------------------------------------------------------------

void ParseNode::attach_child(ParseNode* child) {
  DCHECK(child);
  DCHECK(child->prev == nullptr);
  DCHECK(child->next == nullptr);

  if (tail) {
    tail->next = child;
    child->prev = tail;
    tail = child;
  }
  else {
    head = child;
    tail = child;
  }
}

//------------------------------------------------------------------------------

int ParseNode::node_count() const {
  int accum = 1;
  for (auto c = head; c; c = c->next) {
    accum += c->node_count();
  }
  return accum;
}

//------------------------------------------------------------------------------

ParseNode* ParseNode::left_neighbor() {
  return (_tok_a - 1)->get_span();
}

ParseNode* ParseNode::right_neighbor() {
  return (_tok_b + 1)->get_span();
}

//------------------------------------------------------------------------------

void ParseNode::print_class_name(int max_len) const {
  const char* name = typeid(*this).name();
  int name_len = 0;
  if (sscanf(name, "%d", &name_len)) {
    while((*name >= '0') && (*name <= '9')) name++;
  }
  if (name_len > max_len) name_len = max_len;

  for (int i = 0; i < name_len; i++) {
    putc(name[i], stdout);
  }
  for (int i = name_len; i < max_len; i++) {
    putc(' ', stdout);
  }
}

//------------------------------------------------------------------------------

void ParseNode::check_sanity() {
  // All our children should be sane.
  for (auto cursor = head; cursor; cursor = cursor->next) {
    cursor->check_sanity();
  }

  // Our prev/next pointers should be hooked up correctly
  CHECK(!next || next->prev == this);
  CHECK(!prev || prev->next == this);

  ParseNode* cursor = nullptr;

  // Check node chain
  for (cursor = head; cursor && cursor->next; cursor = cursor->next);
  CHECK(cursor == tail);

  for (cursor = tail; cursor && cursor->prev; cursor = cursor->prev);
  CHECK(cursor == head);
}

//------------------------------------------------------------------------------

Token* ParseNode::tok_a() { return _tok_a; }
Token* ParseNode::tok_b() { return _tok_b; }
const Token* ParseNode::tok_a() const { return _tok_a; }
const Token* ParseNode::tok_b() const { return _tok_b; }

//------------------------------------------------------------------------------

std::string escape_span(const ParseNode* n) {
  if (!n->tok_a() || !n->tok_b()) {
    return "<bad span>";
  }

  auto len = n->tok_b()->get_lex_debug()->span_b - n->tok_a()->get_lex_debug()->span_a;

  std::string result;
  for (auto i = 0; i < len; i++) {
    auto c = n->tok_a()->debug_span_a()[i];
    if (c == '\n') {
      result.push_back('\\');
      result.push_back('n');
    }
    else if (c == '\r') {
      result.push_back('\\');
      result.push_back('r');
    }
    else if (c == '\t') {
      result.push_back('\\');
      result.push_back('t');
    }
    else {
      result.push_back(c);
    }
    if (result.size() >= 80) break;
  }

  return result;
}

//------------------------------------------------------------------------------

void ParseNode::dump_tree(int max_depth, int indentation) {
  const ParseNode* n = this;
  if (max_depth && indentation == max_depth) return;

  printf("%p {%p-%p} ", n, n->tok_a(), n->tok_b());

  printf("{%-40.40s}", escape_span(n).c_str());


  for (int i = 0; i < indentation; i++) printf(" | ");

  if (n->precedence) {
    printf("[%02d %2d] ",
      n->precedence,
      n->assoc
      //n->assoc > 0 ? '>' : n->assoc < 0 ? '<' : '-'
    );
  }
  else {
    printf("[-O-] ");
  }

  if (n->tok_a()) set_color(n->tok_a()->type_to_color());
  //if (!field.empty()) printf("%-10.10s : ", field.c_str());

  n->print_class_name(20);
  set_color(0);
  printf("\n");

  for (auto c = n->head; c; c = c->next) {
    c->dump_tree(max_depth, indentation + 1);
  }
}

//------------------------------------------------------------------------------
