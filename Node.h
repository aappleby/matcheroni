#pragma once
#include <assert.h>
#include "c_lexer.h"
#include <stdio.h>
#include <string>

#include "Tokens.h"

//------------------------------------------------------------------------------

enum NodeType {
  NODE_INVALID = 0,
  NODE_ACCESS_SPECIFIER,
  NODE_CLASS_DECLARATION,
  NODE_CLASS_DEFINITION,
  NODE_COMPOUND_STATEMENT,
  NODE_CONSTANT,
  NODE_CONSTRUCTOR,
  NODE_DECLARATION,
  NODE_DECLTYPE,
  NODE_FIELD_DECLARATION_LIST,
  NODE_FUNCTION_DEFINITION,
  NODE_IDENTIFIER,
  NODE_IF_STATEMENT,
  NODE_PARAMETER_LIST,
  NODE_PARENTHESIZED_EXPRESSION,
  NODE_PREPROC,
  NODE_PUNCT,
  NODE_SPECIFIER,
  NODE_SPECIFIER_LIST,
  NODE_TEMPLATE_DECLARATION,
  NODE_TEMPLATE_PARAMETER_LIST,
  NODE_TRANSLATION_UNIT,
  NODE_TYPEDEF_NAME,
  NODE_TYPEOF_SPECIFIER,
  NODE_WHILE_STATEMENT,

  NODE_INFIX_EXPRESSION,
  NODE_PREFIX_EXPRESSION,
  NODE_POSTFIX_EXPRESSION,
  NODE_OPERATOR,
};

//------------------------------------------------------------------------------

inline NodeType tok_to_node(TokenType t) {
  switch(t) {
    case TOK_STRING:     return NODE_CONSTANT;
    case TOK_IDENTIFIER: return NODE_IDENTIFIER;
    case TOK_PREPROC:    return NODE_PREPROC;
    case TOK_FLOAT:      return NODE_CONSTANT;
    case TOK_INT:        return NODE_CONSTANT;
    case TOK_PUNCT:      return NODE_PUNCT;
    case TOK_CHAR:       return NODE_CONSTANT;
  }
  assert(false);
  return NODE_INVALID;
}

//------------------------------------------------------------------------------

inline const char* node_to_str(NodeType t) {
  switch(t) {
    case NODE_INVALID: return "NODE_INVALID";

    case NODE_ACCESS_SPECIFIER: return "NODE_ACCESS_SPECIFIER";
    case NODE_CLASS_DECLARATION: return "NODE_CLASS_DECLARATION";
    case NODE_CLASS_DEFINITION: return "NODE_CLASS_DEFINITION";
    case NODE_COMPOUND_STATEMENT: return "NODE_COMPOUND_STATEMENT";
    case NODE_CONSTANT: return "NODE_CONSTANT";
    case NODE_CONSTRUCTOR: return "NODE_CONSTRUCTOR";
    case NODE_DECLARATION: return "NODE_DECLARATION";
    case NODE_DECLTYPE: return "NODE_DECLTYPE";
    case NODE_FIELD_DECLARATION_LIST: return "NODE_FIELD_DECLARATION_LIST";
    case NODE_FUNCTION_DEFINITION: return "NODE_FUNCTION_DEFINITION";
    case NODE_IDENTIFIER: return "NODE_IDENTIFIER";
    case NODE_IF_STATEMENT: return "NODE_IF_STATEMENT";
    case NODE_PARAMETER_LIST: return "NODE_PARAMETER_LIST";
    case NODE_PARENTHESIZED_EXPRESSION: return "NODE_PARENTHESIZED_EXPRESSION";
    case NODE_PREPROC: return "NODE_PREPROC";
    case NODE_PUNCT: return "NODE_PUNCT";
    case NODE_SPECIFIER: return "NODE_SPECIFIER";
    case NODE_SPECIFIER_LIST: return "NODE_SPECIFIER_LIST";
    case NODE_TEMPLATE_DECLARATION: return "NODE_TEMPLATE_DECLARATION";
    case NODE_TEMPLATE_PARAMETER_LIST: return "NODE_TEMPLATE_PARAMETER_LIST";
    case NODE_TRANSLATION_UNIT: return "NODE_TRANSLATION_UNIT";
    case NODE_TYPEDEF_NAME: return "NODE_TYPEDEF_NAME";
    case NODE_TYPEOF_SPECIFIER: return "NODE_TYPEOF_SPECIFIER";
    case NODE_WHILE_STATEMENT: return "NODE_WHILE_STATEMENT";

    case NODE_INFIX_EXPRESSION: return "NODE_INFIX_EXPRESSION";
    case NODE_OPERATOR: return "NODE_OPERATOR";
    case NODE_PREFIX_EXPRESSION: return "NODE_PREFIX_EXPRESSION";
    case NODE_POSTFIX_EXPRESSION: return "NODE_POSTFIX_EXPRESSION";
  }
  return "<unknown node>";
}

//------------------------------------------------------------------------------

struct Node {

  Node(NodeType node_type, const Token* tok_a = nullptr, const Token* tok_b = nullptr) {
    assert(node_type != NODE_INVALID);
    if (tok_a == nullptr && tok_b == nullptr) {
    }
    else {
      assert(tok_b > tok_a);
    }

    this->node_type = node_type;
    this->tok_a = tok_a;
    this->tok_b = tok_b;
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

  void append(Node* node) {
    if (!node) return;

    node->parent = this;

    if (tail) {
      tail->next = node;
      node->prev = tail;
      tail = node;
    }
    else {
      head = node;
      tail = node;
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

    Node* cursor = nullptr;

    // Check node chain
    for (cursor = head; cursor && cursor != tail; cursor = cursor->next);
    assert(cursor == tail);

    for (cursor = tail; cursor && cursor != head; cursor = cursor->prev);
    assert(cursor == head);
  }

  //----------------------------------------

  void dump_span() {
    if (!tok_a || !tok_b) {
      printf("<bad span>");
    }
    auto lex_a = tok_a->lex;
    auto lex_b = (tok_b - 1)->lex;

    auto len = lex_b->span_b - lex_a->span_a;
    if (len > 40) len = 40;
    for (auto i = 0; i < len; i++) {
      putc(lex_a->span_a[i], stdout);
    }
    fflush(stdout);
  }

  virtual void dump_tree(int indentation = 0) {
    for (int i = 0; i < indentation; i++) printf(" | ");
    if (!field.empty()) printf("%-10.10s : ", field.c_str());
    printf("%s", node_to_str(node_type));

    if (tok_a && tok_b) {
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

  const Token* tok_a;
  const Token* tok_b;

  std::string field;

  Node* parent = nullptr;
  Node* prev   = nullptr;
  Node* next   = nullptr;
  Node* head   = nullptr;
  Node* tail   = nullptr;
};

//------------------------------------------------------------------------------
