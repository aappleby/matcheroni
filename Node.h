#pragma once
#include <assert.h>
#include "c_lexer.h"
#include <stdio.h>
#include <string>

struct Node;

void dump_tree(Node* head, int max_depth = 1000, int indentation = 0);

//------------------------------------------------------------------------------

enum NodeType {
  NODE_INVALID = 0,
  NODE_ACCESS_SPECIFIER,
  NODE_BINARY_EXPRESSION,
  NODE_BINARY_OP,
  NODE_CLASS_DECLARATION,
  NODE_CLASS_DEFINITION,
  NODE_COMPOUND_STATEMENT,
  NODE_CONSTANT,
  NODE_CONSTRUCTOR,
  NODE_DECLARATION_SPECIFIER,
  NODE_DECLARATION_SPECIFIERS,
  NODE_DECLARATION,
  NODE_DECLTYPE,
  NODE_FIELD_DECLARATION_LIST,
  NODE_IDENTIFIER,
  NODE_IF_STATEMENT,
  NODE_PARAMETER_LIST,
  NODE_PARENTHESIZED_EXPRESSION,
  NODE_PREPROC,
  NODE_PUNCT,
  NODE_SPECIFIER_LIST,
  NODE_TEMPLATE_DECLARATION,
  NODE_TEMPLATE_PARAMETER_LIST,
  NODE_TRANSLATION_UNIT,
  NODE_TYPEDEF_NAME,
  NODE_TYPEOF_SPECIFIER,
  NODE_UNARY_EXPRESSION,
  NODE_UNARY_OP,
};

inline const char* tok_to_str(NodeType t) {
  switch(t) {
    case NODE_INVALID: return "NODE_INVALID";

    case NODE_ACCESS_SPECIFIER: return "NODE_ACCESS_SPECIFIER";
    case NODE_BINARY_EXPRESSION: return "NODE_BINARY_EXPRESSION";
    case NODE_BINARY_OP: return "NODE_BINARY_OP";
    case NODE_CLASS_DECLARATION: return "NODE_CLASS_DECLARATION";
    case NODE_CLASS_DEFINITION: return "NODE_CLASS_DEFINITION";
    case NODE_COMPOUND_STATEMENT: return "NODE_COMPOUND_STATEMENT";
    case NODE_CONSTANT: return "NODE_CONSTANT";
    case NODE_CONSTRUCTOR: return "NODE_CONSTRUCTOR";
    case NODE_DECLARATION_SPECIFIER: return "NODE_DECLARATION_SPECIFIER";
    case NODE_DECLARATION_SPECIFIERS: return "NODE_DECLARATION_SPECIFIERS";
    case NODE_DECLARATION: return "NODE_DECLARATION";
    case NODE_DECLTYPE: return "NODE_DECLTYPE";
    case NODE_FIELD_DECLARATION_LIST: return "NODE_FIELD_DECLARATION_LIST";
    case NODE_IDENTIFIER: return "NODE_IDENTIFIER";
    case NODE_IF_STATEMENT: return "NODE_IF_STATEMENT";
    case NODE_PARAMETER_LIST: return "NODE_PARAMETER_LIST";
    case NODE_PARENTHESIZED_EXPRESSION: return "NODE_PARENTHESIZED_EXPRESSION";
    case NODE_PREPROC: return "NODE_PREPROC";
    case NODE_PUNCT: return "NODE_PUNCT";
    case NODE_SPECIFIER_LIST: return "NODE_SPECIFIER_LIST";
    case NODE_TEMPLATE_DECLARATION: return "NODE_TEMPLATE_DECLARATION";
    case NODE_TEMPLATE_PARAMETER_LIST: return "NODE_TEMPLATE_PARAMETER_LIST";
    case NODE_TRANSLATION_UNIT: return "NODE_TRANSLATION_UNIT";
    case NODE_TYPEDEF_NAME: return "NODE_TYPEDEF_NAME";
    case NODE_TYPEOF_SPECIFIER: return "NODE_TYPEOF_SPECIFIER";
    case NODE_UNARY_EXPRESSION: return "NODE_UNARY_EXPRESSION";
    case NODE_UNARY_OP: return "NODE_UNARY_OP";
  }
  return "<token>";
}

//------------------------------------------------------------------------------

struct Node {

  Node(NodeType node_type, const Lexeme* lex_a = nullptr, const Lexeme* lex_b = nullptr) {
    assert(node_type != NODE_INVALID);
    if (lex_a == nullptr && lex_b == nullptr) {
    }
    else {
      assert(lex_b > lex_a);
    }

    this->node_type = node_type;
    this->lex_a = lex_a;
    this->lex_b = lex_b;
  };

  ~Node() {
    auto cursor = head;
    while(cursor) {
      auto next = cursor->next;
      delete cursor;
      cursor = next;
    }
  }

  //----------------------------------------

  Node* child(int index) {
    auto cursor = head;
    for (auto i = 0; i < index; i++) {
      if (cursor) cursor = cursor->next;
    }
    return cursor;
  }

  //----------------------------------------

  void append(Node* tok) {
    if (!tok) return;

    tok->parent = this;

    if (tail) {
      tail->next = tok;
      tok->prev = tail;
      tail = tok;
    }
    else {
      head = tok;
      tail = tok;
    }
  }

  //----------------------------------------

  void sanity() {
    // All our children should be sane.
    for (auto cursor = head; cursor; cursor = cursor->next) {
      cursor->sanity();
    }

    // Our prev/next pointers should be hooked up correctly
    assert(!next || next->prev == this);
    assert(!prev || prev->next == this);

    //assert(!lex_prev || lex_prev->lex_next == this);
    ///assert(!lex_next || lex_next->lex_prev == this);


    // Our span should exactly enclose our children.
    //assert(!lex_prev || lex_prev->span_b == span_a);
    //assert(!lex_next || lex_next->span_a == span_b);
    //assert(!tok_head || tok_head->span_a == span_a);
    //assert(!tok_tail || tok_tail->span_b == span_b);

    Node* cursor = nullptr;

    // Check token chain
    for (cursor = head; cursor && cursor != tail; cursor = cursor->next);
    assert(cursor == tail);

    for (cursor = tail; cursor && cursor != head; cursor = cursor->prev);
    assert(cursor == head);
  }

  //----------------------------------------

  void dump_span() {
    if (!lex_a || !lex_b) {
      printf("<bad span>");
    }
    auto len = (lex_b - 1)->span_b - lex_a->span_a;
    if (len > 40) len = 40;
    for (auto i = 0; i < len; i++) {
      putc(lex_a->span_a[i], stdout);
    }
  }

  virtual void dump_tree(int indentation = 0) {
    for (int i = 0; i < indentation; i++) printf("  ");
    if (!field.empty()) printf("%-10.10s : ", field.c_str());
    printf("%s", tok_to_str(node_type));

    if (lex_a && lex_b) {
      printf(" '");
      dump_span();
      printf("' ");
    }

    printf("\n");
    for (auto c = head; c; c = c->next) {
      c->dump_tree(indentation + 1);
    }
  }

  //----------------------------------------

  NodeType node_type;

  const Lexeme* lex_a;
  const Lexeme* lex_b;

  std::string field;

  Node* parent = nullptr;
  Node* prev   = nullptr;
  Node* next   = nullptr;
  Node* head   = nullptr;
  Node* tail   = nullptr;
};

//------------------------------------------------------------------------------
