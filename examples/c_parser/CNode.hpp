// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#include <cstddef>  // for size_t
#include <typeinfo>
#include <string>

#include "matcheroni/Parseroni.hpp"
#include "examples/c_lexer/CToken.hpp"

typedef matcheroni::Span<CToken> TokenSpan;

//------------------------------------------------------------------------------

struct CNode : public matcheroni::NodeBase<CNode, CToken> {
  using AtomType = CToken;
  using SpanType = matcheroni::Span<CToken>;

  const char* text_head() const { return span.begin->text.begin; }
  const char* text_tail() const { return (span.end - 1)->text.end; }

  void debug_dump(std::string& out) {
    out += "[";
    out += match_name;
    out += ":";
    if (child_head) {
      for (auto c = child_head; c; c = c->node_next) {
        c->debug_dump(out);
      }
    }
    else {
      out += '`';
      out += std::string(span.begin->text.begin, (span.end - 1)->text.end);
      out += '`';
    }
    out += "]";
  }


  //----------------------------------------

  /*
  template <typename P>
  bool is_a() const {
    return typeid(*this) == typeid(P);
  }

  template <typename P>
  P* child() {
    for (auto cursor = child_head; cursor; cursor = cursor->node_next) {
      if (cursor->is_a<P>()) {
        return dynamic_cast<P*>(cursor);
      }
    }
    return nullptr;
  }

  template <typename P>
  const P* child() const {
    for (auto cursor = child_head; cursor; cursor = cursor->node_next) {
      if (cursor->is_a<P>()) {
        return dynamic_cast<const P*>(cursor);
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
  */

  //----------------------------------------

  int precedence = 0;

  // -2 = prefix, -1 = right-to-left, 0 = none, 1 = left-to-right, 2 = suffix
  int assoc = 0;
};

//------------------------------------------------------------------------------
