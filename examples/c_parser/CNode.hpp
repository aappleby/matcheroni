// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#include <cstddef>  // for size_t
#include <typeinfo>

#include "matcheroni/Parseroni.hpp"
#include "examples/c_lexer/CToken.hpp"

typedef matcheroni::Span<CToken> lex_span;

//------------------------------------------------------------------------------

struct CNode : public matcheroni::NodeBase<CToken> {

  using base = matcheroni::NodeBase<CToken>;

  using NodeType = CNode;

  CNode* child(const char* name) {
    return (CNode*)base::child(name);
  }

  NodeType* node_prev() { return (NodeType*)_node_prev; }
  NodeType* node_next() { return (NodeType*)_node_next; }

  const NodeType* node_prev() const { return (NodeType*)_node_prev; }
  const NodeType* node_next() const { return (NodeType*)_node_next; }

  NodeType* child_head() { return (NodeType*)_child_head; }
  NodeType* child_tail() { return (NodeType*)_child_tail; }

  const NodeType* child_head() const { return (NodeType*)_child_head; }
  const NodeType* child_tail() const { return (NodeType*)_child_tail; }

  //----------------------------------------

  template <typename P>
  bool is_a() const {
    return typeid(*this) == typeid(P);
  }

  template <typename P>
  P* child() {
    for (auto cursor = child_head(); cursor; cursor = cursor->node_next()) {
      if (((CNode*)cursor)->is_a<P>()) {
        return dynamic_cast<P*>(cursor);
      }
    }
    return nullptr;
  }

  template <typename P>
  const P* child() const {
    for (auto cursor = child_head(); cursor; cursor = cursor->node_next()) {
      if (((CNode*)cursor)->is_a<P>()) {
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

  //----------------------------------------

  int precedence = 0;

  // -2 = prefix, -1 = right-to-left, 0 = none, 1 = left-to-right, 2 = suffix
  int assoc = 0;
};

//------------------------------------------------------------------------------
