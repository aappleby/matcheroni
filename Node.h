#pragma once
#include <assert.h>
#include "c_lexer.h"
#include <stdio.h>
#include <string>
#include <typeinfo>

#include "Tokens.h"

void set_color(uint32_t c);

//------------------------------------------------------------------------------

enum NodeType {
  NODE_INVALID = 0,

  NODE_ACCESS_SPECIFIER,
  NODE_ARGUMENT_LIST,
  NODE_ARRAY_EXPRESSION,
  NODE_ARRAY_SUFFIX,
  NODE_ASSIGNMENT_EXPRESSION,
  NODE_ASSIGNMENT_STATEMENT,
  NODE_CALL_EXPRESSION,
  NODE_CASE_STATEMENT,
  NODE_CLASS_DECLARATION,
  NODE_CLASS_DEFINITION,
  NODE_COMPOUND_STATEMENT,
  NODE_CONSTANT,
  NODE_CONSTRUCTOR,
  NODE_DECLARATION_STATEMENT,
  NODE_DECLARATION,
  NODE_DECLTYPE,
  NODE_DEFAULT_STATEMENT,
  NODE_ENUM_DECLARATION,
  NODE_ENUMERATOR_LIST,
  NODE_EXPRESSION,
  NODE_EXPRESSION_LIST,
  NODE_EXPRESSION_STATEMENT,
  NODE_FIELD_LIST,
  NODE_FOR_STATEMENT,
  NODE_FUNCTION_DECLARATION,
  NODE_FUNCTION_DEFINITION,
  NODE_IDENTIFIER,
  NODE_IF_STATEMENT,
  NODE_INFIX_EXPRESSION,
  NODE_INITIALIZER,
  NODE_INITIALIZER_LIST,
  NODE_NAMESPACE_DECLARATION,
  NODE_NAMESPACE_DEFINITION,
  NODE_OPERATOR,
  NODE_PARAMETER_LIST,
  NODE_PARENTHESIZED_EXPRESSION,
  NODE_POSTFIX_EXPRESSION,
  NODE_PREFIX_EXPRESSION,
  NODE_PREPROC,
  NODE_PUNCT,
  NODE_RETURN_STATEMENT,
  NODE_SCOPED_TYPE,
  NODE_SIZEOF,
  NODE_SPECIFIER_LIST,
  NODE_SPECIFIER,
  NODE_STRUCT_DECLARATION,
  NODE_STRUCT_DEFINITION,
  NODE_SWITCH_STATEMENT,
  NODE_TEMPLATE_DECLARATION,
  NODE_TEMPLATE_PARAMETER_LIST,
  NODE_TEMPLATED_TYPE,
  NODE_TRANSLATION_UNIT,
  NODE_TYPECAST,
  NODE_TYPEDEF_NAME,
  NODE_TYPEOF_SPECIFIER,
  NODE_WHILE_STATEMENT,
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
    case NODE_ARGUMENT_LIST: return "NODE_ARGUMENT_LIST";
    case NODE_ARRAY_EXPRESSION: return "NODE_ARRAY_EXPRESSION";
    case NODE_ARRAY_SUFFIX: return "NODE_ARRAY_SUFFIX";
    case NODE_ASSIGNMENT_EXPRESSION: return "NODE_ASSIGNMENT_EXPRESSION";
    case NODE_ASSIGNMENT_STATEMENT: return "NODE_ASSIGNMENT_STATEMENT";
    case NODE_CALL_EXPRESSION: return "NODE_CALL_EXPRESSION";
    case NODE_CASE_STATEMENT: return "NODE_CASE_STATEMENT";
    case NODE_CLASS_DECLARATION: return "NODE_CLASS_DECLARATION";
    case NODE_CLASS_DEFINITION: return "NODE_CLASS_DEFINITION";
    case NODE_COMPOUND_STATEMENT: return "NODE_COMPOUND_STATEMENT";
    case NODE_CONSTANT: return "NODE_CONSTANT";
    case NODE_CONSTRUCTOR: return "NODE_CONSTRUCTOR";
    case NODE_DECLARATION_STATEMENT: return "NODE_DECLARATION_STATEMENT";
    case NODE_DECLARATION: return "NODE_DECLARATION";
    case NODE_DECLTYPE: return "NODE_DECLTYPE";
    case NODE_DEFAULT_STATEMENT: return "NODE_DEFAULT_STATEMENT";
    case NODE_ENUM_DECLARATION: return "NODE_ENUM_DECLARATION";
    case NODE_ENUMERATOR_LIST: return "NODE_ENUMERATOR_LIST";
    case NODE_EXPRESSION: return "NODE_EXPRESSION";
    case NODE_EXPRESSION_LIST: return "NODE_EXPRESSION_LIST";
    case NODE_EXPRESSION_STATEMENT: return "NODE_EXPRESSION_STATEMENT";
    case NODE_FIELD_LIST: return "NODE_FIELD_LIST";
    case NODE_FOR_STATEMENT: return "NODE_FOR_STATEMENT";
    case NODE_FUNCTION_DECLARATION: return "NODE_FUNCTION_DECLARATION";
    case NODE_FUNCTION_DEFINITION: return "NODE_FUNCTION_DEFINITION";
    case NODE_IDENTIFIER: return "NODE_IDENTIFIER";
    case NODE_IF_STATEMENT: return "NODE_IF_STATEMENT";
    case NODE_INFIX_EXPRESSION: return "NODE_INFIX_EXPRESSION";
    case NODE_INITIALIZER: return "NODE_INITIALIZER";
    case NODE_INITIALIZER_LIST: return "NODE_INITIALIZER_LIST";
    case NODE_NAMESPACE_DECLARATION: return "NODE_NAMESPACE_DECLARATION";
    case NODE_NAMESPACE_DEFINITION: return "NODE_NAMESPACE_DEFINITION";
    case NODE_OPERATOR: return "NODE_OPERATOR";
    case NODE_PARAMETER_LIST: return "NODE_PARAMETER_LIST";
    case NODE_PARENTHESIZED_EXPRESSION: return "NODE_PARENTHESIZED_EXPRESSION";
    case NODE_POSTFIX_EXPRESSION: return "NODE_POSTFIX_EXPRESSION";
    case NODE_PREFIX_EXPRESSION: return "NODE_PREFIX_EXPRESSION";
    case NODE_PREPROC: return "NODE_PREPROC";
    case NODE_PUNCT: return "NODE_PUNCT";
    case NODE_RETURN_STATEMENT: return "NODE_RETURN_STATEMENT";
    case NODE_SCOPED_TYPE: return "NODE_SCOPED_TYPE";
    case NODE_SIZEOF: return "NODE_SIZEOF";
    case NODE_SPECIFIER_LIST: return "NODE_SPECIFIER_LIST";
    case NODE_SPECIFIER: return "NODE_SPECIFIER";
    case NODE_STRUCT_DECLARATION: return "NODE_STRUCT_DECLARATION";
    case NODE_STRUCT_DEFINITION: return "NODE_STRUCT_DEFINITION";
    case NODE_SWITCH_STATEMENT: return "NODE_SWITCH_STATEMENT";
    case NODE_TEMPLATE_DECLARATION: return "NODE_TEMPLATE_DECLARATION";
    case NODE_TEMPLATE_PARAMETER_LIST: return "NODE_TEMPLATE_PARAMETER_LIST";
    case NODE_TEMPLATED_TYPE: return "NODE_TEMPLATED_TYPE";
    case NODE_TRANSLATION_UNIT: return "NODE_TRANSLATION_UNIT";
    case NODE_TYPECAST: return "NODE_TYPECAST";
    case NODE_TYPEDEF_NAME: return "NODE_TYPEDEF_NAME";
    case NODE_TYPEOF_SPECIFIER: return "NODE_TYPEOF_SPECIFIER";
    case NODE_WHILE_STATEMENT: return "NODE_WHILE_STATEMENT";
  }
  return "<unknown node>";
}

//------------------------------------------------------------------------------

struct NodeBase {

  NodeBase(NodeType type, const Token* a = nullptr, const Token* b = nullptr) {
    //assert(node_type != NODE_INVALID);
    if (a == nullptr && b == nullptr) {
    }
    else {
      assert(b > a);
    }

    this->node_type = type;
    this->tok_a = a;
    this->tok_b = b;
  };

  NodeBase(NodeType type, const Token* a, const Token* b, NodeBase** children, size_t child_count) {
    if (a == nullptr && b == nullptr) {
    }
    else {
      assert(b > a);
    }

    this->node_type = type;
    this->tok_a = a;
    this->tok_b = b;

    for (auto i = 0; i < child_count; i++) {
      append(children[i]);
    }
  }

  virtual ~NodeBase() {
    auto cursor = head;
    while(cursor) {
      auto next = cursor->next;
      delete cursor;
      cursor = next;
    }
  }

  template<typename P>
  bool isa() const {
    //if (this == nullptr) return false;
    return typeid(*this) == typeid(P);
  }

  template<typename P>
  P* as() {
    //if (this == nullptr) return nullptr;
    if (typeid(*this) == typeid(P)) {
      return dynamic_cast<P*>(this);
    }
    else {
      assert(false);
      return nullptr;
    }
  };

  //----------------------------------------

  NodeBase* child(int index) {
    auto cursor = head;
    for (auto i = 0; i < index; i++) {
      if (cursor) cursor = cursor->next;
    }
    return cursor;
  }

  //----------------------------------------

  void append(NodeBase* node) {
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

    NodeBase* cursor = nullptr;

    // Check node chain
    for (cursor = head; cursor && cursor != tail; cursor = cursor->next);
    assert(cursor == tail);

    for (cursor = tail; cursor && cursor != head; cursor = cursor->prev);
    assert(cursor == head);
  }

  //----------------------------------------

  std::string escape_span() {
    if (!tok_a || !tok_b) {
      return "<bad span>";
    }
    auto lex_a = tok_a->lex;
    auto lex_b = (tok_b - 1)->lex;
    auto len = lex_b->span_b - lex_a->span_a;

    std::string result;
    for (auto i = 0; i < len; i++) {
      auto c = lex_a->span_a[i];
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
      if (result.size() >= 40) break;
    }

    return result;
  }

  //----------------------------------------

  virtual void dump_tree(int max_depth = 0, int indentation = 0) {
    if (max_depth && indentation == max_depth) return;

    for (int i = 0; i < indentation; i++) printf(" | ");

    if (tok_a) set_color(tok_a->lex->color());
    if (!field.empty()) printf("%-10.10s : ", field.c_str());
    //printf("%s", node_to_str(node_type));

    const char* name = typeid(*this).name();
    while((*name >= '0') && (*name <= '9')) name++;
    printf("%s", name);

    printf(" '%s'\n", escape_span().c_str());

    if (tok_a) set_color(0);

    for (auto c = head; c; c = c->next) {
      c->dump_tree(max_depth, indentation + 1);
    }
  }

  //----------------------------------------

  NodeType node_type;

  const Token* tok_a;
  const Token* tok_b;

  std::string field;

  NodeBase* parent = nullptr;
  NodeBase* prev   = nullptr;
  NodeBase* next   = nullptr;
  NodeBase* head   = nullptr;
  NodeBase* tail   = nullptr;
};

//------------------------------------------------------------------------------

struct NodeDispenser {

  NodeDispenser(NodeBase** children, size_t child_count) {
    this->children = children;
    this->child_count = child_count;
  }

  template<typename P>
  operator P*() {
    if (child_count == 0) return nullptr;
    if (children[0] == nullptr) return nullptr;
    if (children[0]->isa<P>()) {
      P* result = children[0]->as<P>();
      children++;
      child_count--;
      return result;
    }
    else {
      return nullptr;
    }
  }

  NodeBase** children;
  size_t child_count;
};

//------------------------------------------------------------------------------
