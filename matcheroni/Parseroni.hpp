#pragma once
#include <stdlib.h>  // for malloc/free
#include <string.h>

#include "matcheroni/Matcheroni.hpp"

//#define PARSERONI_FAST_MODE

namespace matcheroni {

//------------------------------------------------------------------------------

struct SlabAlloc {
  struct Slab {
    Slab* prev;
    Slab* next;
    int cursor;
    int highwater;
    char buf[];
  };

  struct SlabState {
    Slab* top_slab;
    int   top_cursor;
    int   current_size;
  };

  SlabState save_state() {
    return {
      top_slab,
      top_slab->cursor,
      current_size
    };
  }

  void restore_state(SlabState s) {
    top_slab = s.top_slab;
    top_slab->cursor = s.top_cursor;
    current_size = s.current_size;
  }

  // slab size is 1 hugepage. seems to work ok.
  static constexpr int header_size = sizeof(Slab);
  static constexpr int slab_size = 2 * 1024 * 1024 - header_size;

  SlabAlloc() { add_slab(); }

  void reset() {
    if (!top_slab) add_slab();
    while (top_slab->prev) top_slab = top_slab->prev;
    for (auto c = top_slab; c; c = c->next) {
      c->cursor = 0;
    }
    current_size = 0;
  }

  void destroy() {
    reset();
    auto c = top_slab;
    while (c) {
      auto next = c->next;
      ::free((void*)c);
      c = next;
    }
    top_slab = nullptr;
  }

  void add_slab() {
    if (top_slab && top_slab->next) {
      top_slab = top_slab->next;
      // DCHECK(top_slab->cursor == 0);
      return;
    }

    auto new_slab = (Slab*)malloc(header_size + slab_size);
    new_slab->prev = nullptr;
    new_slab->next = nullptr;
    new_slab->cursor = 0;
    new_slab->highwater = 0;

    if (top_slab) top_slab->next = new_slab;
    new_slab->prev = top_slab;
    top_slab = new_slab;
  }

  void* alloc(int size) {
    if (top_slab->cursor + size > slab_size) {
      add_slab();
    }

    auto result = top_slab->buf + top_slab->cursor;
    top_slab->cursor += size;

    current_size += size;
    if (current_size > max_size) max_size = current_size;

    return result;
  }

  void free(void* p, int size) {
    top_slab->cursor -= size;
    if (top_slab->cursor == 0) {
      if (top_slab->prev) {
        top_slab = top_slab->prev;
      }
    }
    current_size -= size;
  }

  static SlabAlloc& slabs() {
    static SlabAlloc inst;
    return inst;
  }

  Slab* top_slab = nullptr;
  int current_size = 0;
  int max_size = 0;
  int refcount = 0;
};

//------------------------------------------------------------------------------

struct NodeBase {
  NodeBase() {
    constructor_calls++;
  }

  virtual ~NodeBase() {
    destructor_calls++;
  }

  static void* operator new(size_t s) { return SlabAlloc::slabs().alloc(s); }
  static void* operator new[](size_t s) { return SlabAlloc::slabs().alloc(s); }
  static void operator delete(void* p, size_t s) { SlabAlloc::slabs().free(p, s); }
  static void operator delete[](void* p, size_t s) { SlabAlloc::slabs().free(p, s); }

  //----------------------------------------

  void init(const char* match_name /*, cspan span*/) {
    this->match_name = match_name;
    //this->span = span;
    this->flags = 0;
  }

  NodeBase* child(const char* name) {
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

  inline static size_t constructor_calls = 0;
  inline static size_t destructor_calls = 0;

  const char* match_name;

  NodeBase* node_prev;
  NodeBase* node_next;

  NodeBase* child_head;
  NodeBase* child_tail;

  int flags;
};

//------------------------------------------------------------------------------

template<typename atom>
struct SpanBase : public NodeBase {
  Span<atom> span;
};

//------------------------------------------------------------------------------

struct Context {
  Context() {
    SlabAlloc::slabs().reset();
  }

  virtual ~Context() {
    reset();
    SlabAlloc::slabs().destroy();
  }

  void reset() {
    auto c = top_tail;
    while (c) {
      auto prev = c->node_prev;
      recycle(c);
      c = prev;
    }

    top_head = nullptr;
    top_tail = nullptr;
    //highwater = nullptr;

    SlabAlloc::slabs().reset();
    NodeBase::constructor_calls = 0;
    NodeBase::destructor_calls = 0;
  }

  //----------------------------------------

  void append(NodeBase* new_node) {
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

  void detach(NodeBase* n) {
    if (n->node_prev) n->node_prev->node_next = n->node_next;
    if (n->node_next) n->node_next->node_prev = n->node_prev;
    if (top_head == n) top_head = n->node_next;
    if (top_tail == n) top_tail = n->node_prev;
    n->node_prev = nullptr;
    n->node_next = nullptr;
  }

  //----------------------------------------

  void splice(NodeBase* new_node, NodeBase* child_head, NodeBase* child_tail) {
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

  void add(NodeBase* new_node, NodeBase* old_tail) {
    // Move all nodes in (old_tail,new_tail] to be children of new_node and
    // append new_node to the node list.
    if (old_tail == top_tail) {
      append(new_node);
    } else {
      auto child_head = old_tail ? old_tail->node_next : top_head;
      auto child_tail = top_tail;
      splice(new_node, child_head, child_tail);
    }
  }

  //----------------------------------------

  template <typename NodeType>
  NodeType* create(const char* match_name, NodeBase* old_tail) {
    auto new_node = new NodeType();
    new_node->init(match_name);

    //highwater = s.b;

    add(new_node, old_tail);

    return new_node;
  }

  //----------------------------------------
  // Nodes _must_ be deleted in the reverse order they were allocated.
  // In practice, this means we must delete the "parent" node first and then
  // must delete the child nodes from tail to head.

#ifndef FAST_MODE
  void recycle(NodeBase* node) {
    if (node == nullptr) return;

    auto tail = node->child_tail;

    delete node;

    while (tail) {
      auto prev = tail->node_prev;
      recycle(tail);
      tail = prev;
    }
  }
#endif

  //----------------------------------------
  // There's one critical detail we need to make the factory work correctly - if
  // we get partway through a match and then fail for some reason, we must
  // "rewind" our match state back to the start of the failed match. This means
  // we must also throw away any parse nodes that were created during the failed
  // match.

#if 0
  template<typename atom>
  void rewind(Span<atom> s) {
    //printf("rewind\n");
    while (top_tail && (top_tail->span.b > s.a)) {
      auto dead = top_tail;
      top_tail = top_tail->node_prev;
#ifndef FAST_MODE
      recycle(dead);
#endif
    }
  }
#endif

  //----------------------------------------

  size_t node_count() {
    size_t accum = 0;
    for (auto c = top_head; c; c = c->node_next) accum += c->node_count();
    return accum;
  }

  //----------------------------------------

  NodeBase* top_head = nullptr;
  NodeBase* top_tail = nullptr;
  //const char* highwater = nullptr;
  int trace_depth = 0;
};

//------------------------------------------------------------------------------
// Matcheroni's default rewind callback does nothing, but if we provide a
// specialized version of it Matcheroni will call it as needed.

#if 0
template <typename atom>
inline void parser_rewind(void* ctx, cspan s) {
  if (ctx) {
    auto context = (Context*)ctx;
    //((Context*)ctx)->rewind(s);

    //printf("rewind\n");
    while (ctx->top_tail && (ctx->top_tail->span.b > s.a)) {
      auto dead = top_tail;
      top_tail = top_tail->node_prev;
#ifndef FAST_MODE
      recycle(dead);
#endif

  }
}
#endif

//------------------------------------------------------------------------------
// To convert our pattern matches to parse nodes, we create a Factory<>
// matcher that constructs a new NodeType() for a successful match, attaches
// any sub-nodes to it, and places it on a node list.

template <StringParam match_name, typename pattern, typename NodeType>
struct Capture {

  template<typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    Context* context = (Context*)ctx;
    auto old_tail = context->top_tail;
#ifdef PARSERONI_FAST_MODE
    auto old_state = SlabAlloc::slabs().save_state();
#endif

    auto end = pattern::match(context, s);

    if (end.is_valid()) {
      Span<atom> node_span = {s.a, end.a};
      NodeType* new_node = (NodeType*)context->create<NodeType>(match_name.str_val, old_tail);
      new_node->span = node_span;
    }
    else {
      context->top_tail = old_tail;
#ifdef PARSERONI_FAST_MODE
      SlabAlloc::slabs().restore_state(old_state);
#endif
    }

    return end;
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

template<typename... rest>
struct CaptureBegin {

  template<typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    Context* context = (Context*)ctx;

    //----------------------------------------
    // Save tail and match pattern

    auto old_tail = context->top_tail;
#ifdef PARSERONI_FAST_MODE
    auto old_state = SlabAlloc::slabs().save_state();
#endif

    auto end = Seq<rest...>::match(context, s);

    if (!end.is_valid()) {
      context->top_tail = old_tail;
#ifdef PARSERONI_FAST_MODE
      SlabAlloc::slabs().restore_state(old_state);
#endif
      return end;
    }

    //----------------------------------------
    // Scan down the node list to find the bookmark

    auto c = old_tail ? old_tail->node_next : context->top_head;
    for (; c; c = c->node_next) {
      if (c->flags & 1) {
        break;
      }
    }
    if (c == nullptr) {
      // No bookmark = no capture, but _not_ a failure
      return end;
    }

    //----------------------------------------
    // Resize the bookmark's span and clear its bookmark flag

    c->span.a = s.a;
    c->flags &= ~1;

    //----------------------------------------
    // Enclose its children

    if (c->node_prev != old_tail) {
      auto child_head = old_tail ? old_tail->node_next : context->top_head;
      auto child_tail = c->node_prev;
      context->detach(c);
      context->splice(c, child_head, child_tail);
    }

    return end;
  }
};

//----------------------------------------

template<StringParam match_name, typename P, typename NodeType>
struct CaptureEnd {
  template<typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    Context* context = (Context*)ctx;

    auto end = P::match(ctx, s);
    if (end.is_valid()) {
      Span<atom> new_span(end.a, end.a);
      NodeType* n = (NodeType*)context->create<NodeType>(match_name.str_val, context->top_tail);
      n->span = new_span;
      n->flags |= 1;
      return end;
    }
    else {
      return end.fail();
    }
  }
};

//------------------------------------------------------------------------------

}; // namespace matcheroni
