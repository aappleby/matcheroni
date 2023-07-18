// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#include <cstddef>  // for size_t
#include <typeinfo>

#include "matcheroni/Parseroni.hpp"
#include "examples/c_lexer/CToken.hpp"

typedef matcheroni::Span<CToken> TokSpan;

//------------------------------------------------------------------------------

struct CNode : public matcheroni::NodeBase<CToken> {

  using base = matcheroni::NodeBase<CToken>;

  using NodeType = CNode;

  CNode* child(const char* name) {
    return (CNode*)base::child(name);
  }

  CNode* node_prev() { return (CNode*)_node_prev; }
  CNode* node_next() { return (CNode*)_node_next; }

  const CNode* node_prev() const { return (CNode*)_node_prev; }
  const CNode* node_next() const { return (CNode*)_node_next; }

  CNode* child_head() { return (CNode*)_child_head; }
  CNode* child_tail() { return (CNode*)_child_tail; }

  const CNode* child_head() const { return (CNode*)_child_head; }
  const CNode* child_tail() const { return (CNode*)_child_tail; }

  //----------------------------------------

  template <typename P>
  bool is_a() const {
    return typeid(*this) == typeid(P);
  }

  template <typename P>
  P* child() {
    for (auto cursor = child_head(); cursor; cursor = cursor->node_next()) {
      if (cursor->is_a<P>()) {
        return dynamic_cast<P*>(cursor);
      }
    }
    return nullptr;
  }

  template <typename P>
  const P* child() const {
    for (auto cursor = child_head(); cursor; cursor = cursor->node_next()) {
      if (cursor->is_a<P>()) {
        return dynamic_cast<const P*>(cursor);
      }
    }
    return nullptr;
  }

  /*
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
  */

  template <typename P>
  P* as_a() {
    return dynamic_cast<P*>(this);
  }

  template <typename P>
  const P* as_a() const {
    return dynamic_cast<const P*>(this);
  }

  void dump_tree(int max_depth = 0, int indentation = 0);

  //----------------------------------------

  int precedence = 0;

  // -2 = prefix, -1 = right-to-left, 0 = none, 1 = left-to-right, 2 = suffix
  int assoc = 0;
};

//------------------------------------------------------------------------------
