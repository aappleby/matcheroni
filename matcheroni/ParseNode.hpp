// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#include <cstddef>  // for size_t
#include <typeinfo>

struct SlabAlloc;
struct Token;

//------------------------------------------------------------------------------

struct ParseNode {
  static SlabAlloc slabs;

  static void* operator new(std::size_t size);
  static void* operator new[](std::size_t size);
  static void operator delete(void*);
  static void operator delete[](void*);

  virtual ~ParseNode();

  bool in_range(const Token* a, const Token* b);
  virtual void init_node(void* ctx, Token* tok_a, Token* tok_b,
                         ParseNode* node_a, ParseNode* node_b);
  virtual void init_leaf(void* ctx, Token* tok_a, Token* tok_b);
  void attach_child(ParseNode* child);
  int node_count() const;
  ParseNode* left_neighbor();
  ParseNode* right_neighbor();

  //----------------------------------------

  template <typename P>
  bool is_a() const {
    return typeid(*this) == typeid(P);
  }

  template <typename P>
  P* child() {
    for (auto cursor = head; cursor; cursor = cursor->next) {
      if (cursor->is_a<P>()) {
        return dynamic_cast<P*>(cursor);
      }
    }
    return nullptr;
  }

  template <typename P>
  const P* child() const {
    for (auto cursor = head; cursor; cursor = cursor->next) {
      if (cursor->is_a<P>()) {
        return dynamic_cast<const P*>(cursor);
      }
    }
    return nullptr;
  }

  template <typename P>
  P* search() {
    if (this->is_a<P>()) return this->as_a<P>();
    for (auto cursor = head; cursor; cursor = cursor->next) {
      if (auto c = cursor->search<P>()) {
        return c;
      }
    }
    return nullptr;
  }

  template <typename P>
  P* as_a() {
    return dynamic_cast<P*>(this);
  }

  template <typename P>
  const P* as_a() const {
    return dynamic_cast<const P*>(this);
  }

  //----------------------------------------

  void print_class_name(int max_len = 0) const;
  void check_sanity();

  Token* tok_a();
  Token* tok_b();
  const Token* tok_a() const;
  const Token* tok_b() const;

  void dump_tree(int max_depth, int indentation);

  //----------------------------------------

  inline static int constructor_count = 0;

  int precedence = 0;

  // -2 = prefix, -1 = right-to-left, 0 = none, 1 = left-to-right, 2 = suffix
  int assoc = 0;

  ParseNode* prev = nullptr;
  ParseNode* next = nullptr;
  ParseNode* head = nullptr;
  ParseNode* tail = nullptr;

 private:
  Token* _tok_a = nullptr;  // First token, inclusivve
  Token* _tok_b = nullptr;  // Last token, inclusive
};

//------------------------------------------------------------------------------

struct ParseNodeIterator {
  ParseNodeIterator(ParseNode* cursor) : n(cursor) {}
  ParseNodeIterator& operator++() {
    n = n->next;
    return *this;
  }
  bool operator!=(ParseNodeIterator& b) const { return n != b.n; }
  ParseNode* operator*() const { return n; }
  ParseNode* n;
};

inline ParseNodeIterator begin(ParseNode* parent) {
  return ParseNodeIterator(parent->head);
}

inline ParseNodeIterator end(ParseNode* parent) {
  return ParseNodeIterator(nullptr);
}

struct ConstParseNodeIterator {
  ConstParseNodeIterator(const ParseNode* cursor) : n(cursor) {}
  ConstParseNodeIterator& operator++() {
    n = n->next;
    return *this;
  }
  bool operator!=(const ConstParseNodeIterator& b) const { return n != b.n; }
  const ParseNode* operator*() const { return n; }
  const ParseNode* n;
};

inline ConstParseNodeIterator begin(const ParseNode* parent) {
  return ConstParseNodeIterator(parent->head);
}

inline ConstParseNodeIterator end(const ParseNode* parent) {
  return ConstParseNodeIterator(nullptr);
}

//------------------------------------------------------------------------------
