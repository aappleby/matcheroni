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

  NODE_TEST,

  NODE_ACCESS_SPECIFIER,
  NODE_BITSIZE,
  NODE_CONSTANT,
  NODE_CONSTRUCTOR,
  NODE_DECLTYPE,
  NODE_IDENTIFIER,
  NODE_INITIALIZER,
  NODE_PREPROC,
  NODE_PUNCT,
  NODE_SCOPED_TYPE,
  NODE_SIZEOF,
  NODE_SPECIFIER,
  NODE_TEMPLATED_TYPE,
  NODE_TRANSLATION_UNIT,
  NODE_TYPEDEF_NAME,
  NODE_TYPE_NAME,
  NODE_TYPEOF_SPECIFIER,
  NODE_ARRAY_SUFFIX,
  NODE_KEYWORD,

  NODE_OPERATOR,

  NODE_EXPRESSION_ATOM,
  NODE_EXPRESSION_UNIT,
  NODE_EXPRESSION_CHAIN,

  // Expression types in precedence order
  NODE_EXPRESSION_PAREN,
  NODE_EXPRESSION_SCOPE,
  NODE_EXPRESSION_POSTINCDEC,
  NODE_EXPRESSION_FCAST,
  NODE_EXPRESSION_CALL,
  NODE_EXPRESSION_SUBSCRIPT,
  NODE_EXPRESSION_MEMBER,
  NODE_EXPRESSION_PREINCDEC,
  NODE_EXPRESSION_PLUSMINUS,
  NODE_EXPRESSION_NOT,
  NODE_EXPRESSION_CAST,
  NODE_EXPRESSION_DEREFERENCE,
  NODE_EXPRESSION_ADDRESSOF,
  NODE_EXPRESSION_SIZEOF,
  NODE_EXPRESSION_PTRTOMEMBER,
  NODE_EXPRESSION_MULDIVMOD,
  NODE_EXPRESSION_ADDSUB,
  NODE_EXPRESSION_SHIFT,
  NODE_EXPRESSION_THREEWAY,
  NODE_EXPRESSION_RELATIONAL,
  NODE_EXPRESSION_EQUALITY,
  NODE_EXPRESSION_BITWISE_AND,
  NODE_EXPRESSION_BITWISE_XOR,
  NODE_EXPRESSION_BITWISE_OR,
  NODE_EXPRESSION_LOGICAL_AND,
  NODE_EXPRESSION_LOGICAL_OR,
  NODE_EXPRESSION_TERNARY,
  NODE_EXPRESSION_ASSIGNMENT,
  NODE_EXPRESSION_COMMA,

  NODE_STATEMENT_ASSIGNMENT,
  NODE_STATEMENT_CASE,
  NODE_STATEMENT_DECL,
  NODE_STATEMENT_DEFAULT,
  NODE_STATEMENT_EXPRESSION,
  NODE_STATEMENT_FOR,
  NODE_STATEMENT_IFELSE,
  NODE_STATEMENT_RETURN,
  NODE_STATEMENT_SWITCH,
  NODE_STATEMENT_TYPEDEF,
  NODE_STATEMENT_WHILE,
  NODE_STATEMENT_COMPOUND,

  NODE_DECLARATION_CLASS,
  NODE_DECLARATION_NAMESPACE,
  NODE_DECLARATION,
  NODE_DECLARATION_ATOM, // FIXME what did the spec call this?
  NODE_DECLARATION_ENUM,
  NODE_DECLARATION_FUNCTION,
  NODE_DECLARATION_STRUCT,
  NODE_DECLARATION_TEMPLATE,

  NODE_DEFINITION_CLASS,
  NODE_DEFINITION_FUNCTION,
  NODE_DEFINITION_NAMESPACE,
  NODE_DEFINITION_STRUCT,

  NODE_LIST_PARAMETER,
  NODE_LIST_ARGUMENT,
  NODE_LIST_INITIALIZER,
  NODE_LIST_SPECIFIER,
  NODE_LIST_TEMPLATE_PARAMETER,
  NODE_LIST_ENUMERATOR,
  NODE_LIST_FIELD,
  NODE_LIST_EXPRESSION
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

struct NodeBase;

struct NodeStack {
  void push(NodeBase* n);
  NodeBase* pop();
  NodeBase* back();
  size_t top() const;
  void dump_top();
  void clear_to(size_t new_top);
  void pop_to(size_t new_top);

  NodeBase*  _stack[256] = {0};
  size_t _top = 0;
};

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
    if (this == nullptr) return false;
    return typeid(*this) == typeid(P);
  }

  template<typename P>
  P* child() {
    if (this == nullptr) return nullptr;
    for (auto cursor = head; cursor; cursor = cursor->next) {
      if (cursor->isa<P>()) {
        return dynamic_cast<P*>(cursor);
      }
    }
    return nullptr;
  }

  template<typename P>
  P* as() {
    if (this == nullptr) return nullptr;
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
    for (cursor = head; cursor && cursor->next; cursor = cursor->next);
    assert(cursor == tail);

    for (cursor = tail; cursor && cursor->prev; cursor = cursor->prev);
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

    const char* name = typeid(*this).name();
    int name_len = 0;
    if (sscanf(name, "%d", &name_len)) {
      while((*name >= '0') && (*name <= '9')) name++;
      for (int i = 0; i < name_len; i++) {
        putc(name[i], stdout);
      }
    }
    else {
      printf("%s", name);
    }

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

  static NodeStack node_stack;
};

//------------------------------------------------------------------------------

template<typename NT>
struct NodeMaker : public NodeBase {
  NodeMaker(const Token* a, const Token* b, NodeBase** children, size_t child_count)
  : NodeBase(NT::node_type, a, b, children, child_count) {
  }

  static const Token* match(const Token* a, const Token* b) {
    auto old_top = node_stack.top();
    auto end = NT::pattern::match(a, b);
    auto new_top = node_stack.top();

    if (end && end != a) {
      auto node = new NT(a, end, &node_stack._stack[old_top], new_top - old_top);
      node_stack.pop_to(old_top);
      node_stack.push(node);
      return end;
    }
    else {
      node_stack.clear_to(old_top);
      return nullptr;
    }
  }
};

//------------------------------------------------------------------------------

template<typename pattern>
struct Dump {
  static const Token* match(const Token* a, const Token* b) {
    auto end = pattern::match(a, b);
    if (end) NodeBase::node_stack.dump_top();
    return end;
  }
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
