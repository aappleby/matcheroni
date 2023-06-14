#pragma once
#include <assert.h>
#include "c_lexer.h"
#include <stdio.h>
#include <string>
#include <typeinfo>
#include <set>
#include <string>

#include "Tokens.h"

typedef std::set<std::string> IdentifierSet;

void set_color(uint32_t c);

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

  void init(const Token* a = nullptr, const Token* b = nullptr) {
    if (a == nullptr && b == nullptr) {
    }
    else {
      assert(b > a);
    }

    this->tok_a = a;
    this->tok_b = b;
  }

  void init(const Token* a, const Token* b, NodeBase** children, size_t child_count) {
    if (a == nullptr && b == nullptr) {
    }
    else {
      assert(b > a);
    }

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
  const P* child() const {
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

  NodeBase* child_at(int index) {
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

  const Token* tok_a = nullptr;
  const Token* tok_b = nullptr;

  std::string field = "";

  NodeBase* parent = nullptr;
  NodeBase* prev   = nullptr;
  NodeBase* next   = nullptr;
  NodeBase* head   = nullptr;
  NodeBase* tail   = nullptr;

  static NodeStack node_stack;

  static inline IdentifierSet global_ids = {};
  static inline IdentifierSet global_types = {
    "void", "char", "short", "int", "long", "float", "double", "signed",
    "unsigned",
  };

};

//------------------------------------------------------------------------------

template<typename P>
struct LogTypename {
  static const Token* match(const Token* a, const Token* b) {
    auto end = P::match(a, b);
    if (end) {
      assert(end == a + 1);
      auto s = std::string(a->lex->span_a, a->lex->span_b);
      NodeBase::global_types.insert(s);
    }
    return end;
  }
};

//------------------------------------------------------------------------------

template<typename NT>
struct NodeMaker : public NodeBase {

  static const Token* match(const Token* a, const Token* b) {
    auto old_top = node_stack.top();
    auto end = NT::pattern::match(a, b);
    auto new_top = node_stack.top();

    if (end && end != a) {
      //auto node = new NT(a, end, &node_stack._stack[old_top], new_top - old_top);
      auto node = new NT();
      node->init(a, end, &node_stack._stack[old_top], new_top - old_top);
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

template<typename NT>
struct Wrapper {
  static const Token* match(const Token* a, const Token* b) {
    return NT::pattern::match(a, b);
  }
};

//------------------------------------------------------------------------------

template<StringParam lit>
struct NodeOperator : public NodeBase {
  static const Token* match(const Token* a, const Token* b) {
    if (auto end = Operator<lit>::match(a, b)) {
      auto n = new NodeOperator();
      n->init(a, end);
      node_stack.push(n);
      return end;
    }
    else {
      return nullptr;
    }
  }
};

//------------------------------------------------------------------------------

template<StringParam lit>
struct NodeKeyword : public NodeBase {
  static const Token* match(const Token* a, const Token* b) {
    if (auto end = Keyword<lit>::match(a, b)) {
      auto n = new NodeKeyword();
      n->init(a, end);
      node_stack.push(n);
      return end;
    }
    else {
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

struct NodeIdentifier : public NodeMaker<NodeIdentifier> {
  using pattern = Atom<TOK_IDENTIFIER>;
};

//------------------------------------------------------------------------------

struct NodeConstant : public NodeMaker<NodeConstant> {
  using pattern = Oneof<
    Atom<TOK_FLOAT>,
    Atom<TOK_INT>,
    Atom<TOK_CHAR>,
    Atom<TOK_STRING>
  >;
};

//------------------------------------------------------------------------------

struct NodeTypeName : public NodeMaker<NodeTypeName> {
  static const Token* match(const Token* a, const Token* b) {
    if (!a || !a->is_identifier()) return nullptr;

    for (auto& t : global_types) {
      if (a->lex->match(t.c_str())) {
        auto n = new NodeTypeName();
        n->init(a, a+1, nullptr, 0);
        node_stack.push(n);
        return a + 1;
      }
    }
    return nullptr;
  }
};

//------------------------------------------------------------------------------
