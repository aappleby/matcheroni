#pragma once
#include <stdlib.h>  // for malloc/free
#include <string.h>
#include <stdint.h>

#include "matcheroni/Matcheroni.hpp"

namespace parseroni {

using namespace matcheroni;

//------------------------------------------------------------------------------
// This is an optimized allocator for Parseroni - it allows for alloc/free, but
// frees must be in LIFO order - if you allocate A, B, and C, you must
// deallocate in C-B-A order.

struct LinearAlloc {
  struct Slab {
    size_t size() { return cursor - buf; }
    void clear() { cursor = buf; }

    Slab* prev;
    Slab* next;
    char* cursor;
    char buf[];
  };

  // Default slab size is 2 megs = 1 hugepage. Seems to work ok.
  static constexpr int header_size = sizeof(Slab);
  static constexpr int slab_size = 2 * 1024 * 1024 - header_size;
  static constexpr int alloc_overhead = 8;

  LinearAlloc() {
    add_slab();
  }

  ~LinearAlloc() {
    reset();
    auto c = top_slab;
    while (c) {
      auto next = c->next;
      ::free((void*)c);
      c = next;
    }
    top_slab = nullptr;
  }


  void reset() {
    while (top_slab->prev) top_slab = top_slab->prev;
    for (auto c = top_slab; c; c = c->next) c->clear();
  }

  void add_slab() {
    if (top_slab && top_slab->next) {
      top_slab = top_slab->next;
      return;
    }

    auto new_slab = (Slab*)malloc(header_size + slab_size);
    new_slab->prev = nullptr;
    new_slab->next = nullptr;
    new_slab->cursor = new_slab->buf;

    if (top_slab) top_slab->next = new_slab;
    new_slab->prev = top_slab;
    top_slab = new_slab;
  }

  void* alloc(int alloc_size) {
    if (top_slab->size() + alloc_size + alloc_overhead > slab_size) {
      add_slab();
    }

    auto result = top_slab->cursor;
    top_slab->cursor += alloc_size;
    *(uint64_t*)(top_slab->cursor) = alloc_size;
    top_slab->cursor += alloc_overhead;

    return result;
  }

  void free(void* p) {
    top_slab->cursor -= alloc_overhead;
    uint64_t alloc_size = *(uint64_t*)top_slab->cursor;
    top_slab->cursor -= alloc_size;

    if (top_slab->size() == 0 && top_slab->prev) {
      top_slab = top_slab->prev;
    }
  }

  int current_size() const {
    auto slab = top_slab;
    while (slab->prev) slab = slab->prev;
    int sum = 0;
    for (; slab; slab = slab->next) {
      sum += slab->size();
    }
    return sum;
  }

  bool is_empty() const {
    return top_slab->prev == nullptr && top_slab->size() == 0;
  }

  Slab* top_slab = nullptr;
};

//------------------------------------------------------------------------------

template<typename NodeType, typename AtomType>
struct NodeBase {
  using SpanType = Span<AtomType>;

  //----------------------------------------

  void init(const char* match_name, SpanType span, uint64_t flags) {
    this->match_name = match_name;
    this->span  = span;
    this->flags = flags;
  }

  NodeType* child(const char* name) {
    for (auto c = child_head; c; c = c->node_next) {
      if (strcmp(name, c->match_name) == 0) return c;
    }
    return nullptr;
  }

  size_t node_count() {
    size_t accum = 1;
    for (auto c = child_head; c; c = c->node_next) accum += c->node_count();
    return accum;
  }

  //----------------------------------------

  const char* match_name;
  SpanType    span;
  uint64_t    flags;

  NodeType*   node_prev;
  NodeType*   node_next;
  NodeType*   child_head;
  NodeType*   child_tail;
};

//------------------------------------------------------------------------------

template<typename _NodeType, bool _call_constructors = true, bool _call_destructors = true>
struct NodeContext {
  using NodeType = _NodeType;
  using SpanType = typename NodeType::SpanType;
  using AtomType = typename SpanType::AtomType;

  static constexpr bool call_constructors = _call_constructors;
  static constexpr bool call_destructors  = _call_destructors;

  NodeContext() {
    top_head = nullptr;
    top_tail = nullptr;
    trace_depth = 0;
  }

  ~NodeContext() {
    reset();
  }

  void reset() {
    // Call destructors for all the nodes in the allocator.
    if (call_destructors) {
      for (auto slab = alloc.top_slab; slab; slab = slab->prev) {
        while(slab->cursor > slab->buf) {
          slab->cursor -= LinearAlloc::alloc_overhead;
          int alloc_size = *(uint64_t*)slab->cursor;
          slab->cursor -= alloc_size;
          NodeType* node = (NodeType*)slab->cursor;
          node->~NodeType();
        }
      }
    }

    top_head = nullptr;
    top_tail = nullptr;
    alloc.reset();
  }

  //----------------------------------------

  size_t node_count() {
    size_t accum = 0;
    for (auto c = top_head; c; c = c->node_next) accum += c->node_count();
    return accum;
  }

  //----------------------------------------

  void append(NodeType* new_node) {
    new_node->node_prev = nullptr;
    new_node->node_next = nullptr;
    new_node->child_head = nullptr;
    new_node->child_tail = nullptr;

    if (top_tail) {
      new_node->node_prev = top_tail;
      top_tail->node_next = new_node;
      top_tail = new_node;
    } else {
      top_head = new_node;
      top_tail = new_node;
    }
  }

  //----------------------------------------

  void detach(NodeType* n) {
    if (n->node_prev) n->node_prev->node_next = n->node_next;
    if (n->node_next) n->node_next->node_prev = n->node_prev;
    if (top_head == n) top_head = n->node_next;
    if (top_tail == n) top_tail = n->node_prev;
    n->node_prev = nullptr;
    n->node_next = nullptr;
  }

  //----------------------------------------

  void splice(NodeType* new_node, NodeType* child_head, NodeType* child_tail) {
    new_node->node_prev = child_head->node_prev;
    new_node->node_next = child_tail->node_next;
    new_node->child_head = child_head;
    new_node->child_tail = child_tail;

    if (child_head->node_prev) child_head->node_prev->node_next = new_node;
    if (child_tail->node_next) child_head->node_next->node_prev = new_node;

    child_head->node_prev = nullptr;
    child_tail->node_next = nullptr;

    if (top_head == child_head) top_head = new_node;
    if (top_tail == child_tail) top_tail = new_node;
  }

  //----------------------------------------
  // If we get partway through a match and then fail for some reason, we must
  // "rewind" our match state back to the start of the failed match. This means
  // we must also throw away any parse nodes that were created during the failed
  // match.

  NodeType* checkpoint() { return top_tail; }

  void rewind(NodeType* old_tail) {
    while(top_tail != old_tail) {
      auto dead = top_tail;
      top_tail = top_tail->node_prev;
      recycle(dead);
    }
  }

  //----------------------------------------
  // FIXME could this be faster if there was an append-only version for
  // captures without children?

  void merge_node(NodeType* new_node, NodeType* old_tail) {

    if (new_node->span.end > _highwater) _highwater = new_node->span.end;

    // Move all nodes in (old_tail,new_tail] to be children of new_node and
    // append new_node to the node list.

    if (old_tail == top_tail) {
      append(new_node);
    } else {
      auto child_head = old_tail ? old_tail->node_next : top_head;
      auto child_tail = top_tail;
      splice(new_node, (NodeType*)child_head, child_tail);
    }
  }

  //----------------------------------------

  NodeType* enclose_bookmark(NodeType* old_tail, NodeType::SpanType bounds) {

    // Scan down the node list to find the bookmark
    auto node_b = old_tail ? old_tail->node_next : top_head;
    for (; node_b; node_b = node_b->node_next) {
      if (node_b->flags & 1) {
        break;
      }
    }

    // No bookmark = no capture, but _not_ a failure
    if (node_b == nullptr) {
      return node_b;
    }

    // Resize the bookmark's span and clear its bookmark flag
    node_b->span = bounds;
    node_b->flags &= ~1;

    // Enclose its children
    if (node_b->node_prev != old_tail) {
      auto child_head = old_tail ? old_tail->node_next : top_head;
      //auto child_head = node_a;
      auto child_tail = node_b->node_prev;
      detach(node_b);
      splice(node_b, child_head, child_tail);
    }

    return node_b;
  }

  //----------------------------------------
  // Nodes _must_ be deleted in the reverse order they were allocated.
  // In practice, this means we must delete the "parent" node first and then
  // must delete the child nodes from tail to head.

  void recycle(NodeType* node) {
    if (node == nullptr) return;
    auto tail = node->child_tail;

    detach(node);
    if (call_destructors) node->~NodeType();
    alloc.free(node);

    while (tail) {
      auto prev = tail->node_prev;
      recycle((NodeType*)tail);
      tail = prev;
    }
  }

  //----------------------------------------

  LinearAlloc alloc;
  NodeType* top_head;
  NodeType* top_tail;
  int trace_depth;
  const SpanType::AtomType* _highwater = nullptr;
};

//------------------------------------------------------------------------------
// To convert our pattern matches to parse nodes, we create a Capture<>
// matcher that constructs a new NodeType() for a successful match, attaches
// any sub-nodes to it, and places it on the context's node list.

template <StringParam match_name, typename pattern, typename node_type>
struct Capture {
  static_assert((sizeof(node_type) & 7) == 0);

  template<typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    auto old_tail = ctx.top_tail;
    auto tail = pattern::match(ctx, body);

    if (tail.is_valid()) {
      Span<atom> node_span = {body.begin, tail.begin};

      node_type* new_node = (node_type*)ctx.alloc.alloc(sizeof(node_type));
      if (context::call_constructors) {
        new (new_node) node_type();
      }
      ctx.merge_node(new_node, old_tail);
      new_node->init(match_name.str_val, node_span, 0);
    }

    return tail;
  }
};

//------------------------------------------------------------------------------
// Suffixes are a problem. A pattern that has to match a bunch of stuff before
// failing due to a suffix can cause an exponential increase in parse time.

// To partly work around this, we can split capture into two pieces:

// CaptureBegin<> behaves like Seq<> and encloses things to capture.

// CaptureEnd<> marks the ending point of a capture and defines the match name
// and type of the capture.

// There should be at most one reachable CaptureEnd per CaptureBegin.

// If no CaptureEnd is hit inside a CaptureBegin, the capture does not occur
// but this is _not_ an error.

template <typename node_type, typename... rest>
struct CaptureBegin {
  static_assert((sizeof(node_type) & 7) == 0);

  template<typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    auto old_tail = ctx.top_tail;
    auto tail = Seq<rest...>::match(ctx, body);
    if (tail.is_valid()) {
      Span<atom> bounds(body.begin, tail.begin);
      ctx.enclose_bookmark(old_tail, bounds);
    }

    return tail;
  }
};

//----------------------------------------

template<StringParam match_name, typename P, typename node_type>
struct CaptureEnd {
  static_assert((sizeof(node_type) & 7) == 0);

  template<typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    auto tail = P::match(ctx, body);
    if (tail.is_valid()) {
      Span<atom> new_span(tail.begin, tail.begin);

      node_type* new_node = (node_type*)ctx.alloc.alloc(sizeof(node_type));
      if (context::call_constructors) {
        new (new_node) node_type();
      }

      ctx.merge_node(new_node, ctx.top_tail);
      new_node->init(match_name.str_val, new_span, /*flags*/ 1);
    }
    return tail;

  }
};

//------------------------------------------------------------------------------
// We'll be parsing text a lot, so these are convenience declarations.

struct TextParseNode : public NodeBase<TextParseNode, char> {
  TextSpan as_text() const { return span; }
};

struct TextParseContext : public NodeContext<TextParseNode> {
  static int atom_cmp(char a, int b) { return (unsigned char)a - b; }
};

//------------------------------------------------------------------------------

}; // namespace parseroni
