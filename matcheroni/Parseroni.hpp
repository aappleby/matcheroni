#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>  // for malloc/free
#include <string.h>

#include <string>

#include "matcheroni/Matcheroni.hpp"

//#define PARSERONI_NO_RECYCLE
//#define PARSERONI_NO_CONSTRUCTORS
//#define PARSERONI_NO_DESTRUCTORS
//#define PARSERONI_NO_REWIND

//------------------------------------------------------------------------------

using cspan = matcheroni::Span<const char>;

inline cspan to_span(const char* text) {
  return cspan(text, text + strlen(text));
}

inline cspan to_span(const std::string& s) {
  return cspan(s.data(), s.data() + s.size());
}

inline std::string to_string(cspan s) {
  return s.a ? std::string(s.a, s.b) : std::string(s.b);
}

inline bool operator==(cspan a, const std::string& b) {
  return to_string(a) == b;
}

inline bool operator==(const std::string& a, cspan b) {
  return a == to_string(b);
}

inline bool operator!=(cspan a, const std::string& b) {
  return to_string(a) != b;
}

inline bool operator!=(const std::string& a, cspan b) {
  return a != to_string(b);
}

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

  void reset() {
    if (!top_slab) add_slab();
    while (top_slab->prev) top_slab = top_slab->prev;
    for (auto c = top_slab; c; c = c->next) {
      c->cursor = 0;
    }
    current_size = 0;
  }

  void add_slab() {
    if (top_slab && top_slab->next) {
      top_slab = top_slab->next;
      // DCHECK(top_slab->cursor == 0);
      return;
    }

    static int count = 0;
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
    auto offset = (char*)p - top_slab->buf;
    // DCHECK(offset + size == top_slab->cursor);
    top_slab->cursor -= size;
    if (top_slab->cursor == 0) {
      if (top_slab->prev) {
        top_slab = top_slab->prev;
      }
    }
    current_size -= size;
    // printf("top_slab->cursor = %ld\n", top_slab->cursor);
  }

  Slab* top_slab;
  int current_size;
  int max_size;
};

//------------------------------------------------------------------------------

struct NodeBase {
  NodeBase(const char* match_name, cspan span)
      : match_name(match_name), span(span), flags(0) {
    constructor_calls++;
  }

  virtual ~NodeBase() {
    destructor_calls++;
  }

  static void* operator new(size_t s) { return slabs.alloc(s); }
  static void* operator new[](size_t s) { return slabs.alloc(s); }
  static void operator delete(void* p, size_t s) { slabs.free(p, s); }
  static void operator delete[](void* p, size_t s) { slabs.free(p, s); }

  //----------------------------------------

  size_t node_count() {
    size_t accum = 1;
    for (auto c = child_head; c; c = c->node_next) accum += c->node_count();
    return accum;
  }

  //----------------------------------------

  uint64_t hash() {
    uint64_t h = 1;

    for (auto c = match_name; *c; c++) {
      h = (h * 975313579) ^ *c;
    }

    if (!child_head) {
      for (auto c = span.a; c < span.b; c++) {
        h = (h * 123456789) ^ *c;
      }
    }

    for (auto c = child_head; c; c = c->node_next) {
      h = (h * 987654321) ^ c->hash();
    }

    return h;
  }

  //----------------------------------------

  inline static SlabAlloc slabs;
  inline static size_t constructor_calls = 0;
  inline static size_t destructor_calls = 0;

  const char* match_name;
  cspan span;

  NodeBase* node_prev;
  NodeBase* node_next;

  NodeBase* child_head;
  NodeBase* child_tail;

  uint64_t flags;
};

//------------------------------------------------------------------------------

struct ParseNodeIterator {
  ParseNodeIterator(NodeBase* cursor) : n(cursor) {}
  ParseNodeIterator& operator++() {
    n = n->node_next;
    return *this;
  }
  bool operator!=(ParseNodeIterator& b) const { return n != b.n; }
  NodeBase* operator*() const { return n; }
  NodeBase* n;
};

inline ParseNodeIterator begin(NodeBase* parent) {
  return ParseNodeIterator(parent->child_head);
}

inline ParseNodeIterator end(NodeBase* parent) {
  return ParseNodeIterator(nullptr);
}

struct ConstParseNodeIterator {
  ConstParseNodeIterator(const NodeBase* cursor) : n(cursor) {}
  ConstParseNodeIterator& operator++() {
    n = n->node_next;
    return *this;
  }
  bool operator!=(const ConstParseNodeIterator& b) const { return n != b.n; }
  const NodeBase* operator*() const { return n; }
  const NodeBase* n;
};

inline ConstParseNodeIterator begin(const NodeBase* parent) {
  return ConstParseNodeIterator(parent->child_head);
}

inline ConstParseNodeIterator end(const NodeBase* parent) {
  return ConstParseNodeIterator(nullptr);
}

//------------------------------------------------------------------------------

inline void print_flat(cspan s, int max_len = 0) {
  if (max_len == 0) max_len = s.len();

  if (!s.is_valid()) {
    printf("<invalid>");
    for (int i = 0; i < max_len - strlen("<invalid>"); i++) putc(' ', stdout);
    return;
  }

  if (s.is_empty()) {
    printf("<empty>");
    for (int i = 0; i < max_len - strlen("<empty>"); i++) putc(' ', stdout);
    return;
  }

  int span_len = max_len;
  if (s.len() > max_len) span_len -= 3;

  for (int i = 0; i < span_len; i++) {
    if (i >= s.len())
      putc(' ', stdout);
    else if (s.a[i] == '\n')
      putc(' ', stdout);
    else if (s.a[i] == '\r')
      putc(' ', stdout);
    else if (s.a[i] == '\t')
      putc(' ', stdout);
    else
      putc(s.a[i], stdout);
  }

  if (s.len() > max_len) printf("...");
}

//------------------------------------------------------------------------------

inline void print_bar(int depth, cspan s, const char* val,
                      const char* suffix) {
  printf("|");
  print_flat(s, 20);
  printf("|");

  printf(depth == 0 ? "  *" : "   ");
  for (int i = 0; i < depth; i++) {
    printf(i == depth - 1 ? "|--" : "|  ");
  }

  printf("%s %s", val, suffix);
  printf("\n");
}

//------------------------------------------------------------------------------
// Prints a text representation of the parse tree.

inline void print_tree(NodeBase* node, int depth = 0) {
  print_bar(depth, node->span, node->match_name, "");
  for (auto c = node->child_head; c; c = c->node_next) {
    print_tree(c, depth + 1);
  }
}

//------------------------------------------------------------------------------

inline void print_match(cspan text, cspan match) {
  printf("Match found:\n");
  print_flat(text);
  printf("\n");

  auto prefix_len = match.a - text.a;
  auto suffix_len = text.b - match.b;

  for (auto i = 0; i < prefix_len;  i++) putc('_', stdout);
  for (auto i = 0; i < match.len(); i++) putc('^', stdout);
  for (auto i = 0; i < suffix_len;  i++) putc('_', stdout);
  printf("\n");
}

//------------------------------------------------------------------------------

inline void print_fail(cspan text, cspan tail) {
  printf("Match failed here:\n");
  print_flat(text);
  printf("\n");

  auto fail_pos = tail.b - text.a;
  auto suffix_len = text.b - tail.b;

  for (int i = 0; i < fail_pos;   i++) putc('_', stdout);
  for (int i = 0; i < suffix_len; i++) putc('^', stdout);
  printf("\n");
}

//------------------------------------------------------------------------------

struct Context {
  Context() {}

  virtual ~Context() {
    reset();
    NodeBase::slabs.destroy();
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
    highwater = nullptr;

    NodeBase::slabs.reset();
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
  NodeType* create(const char* match_name, cspan s, NodeBase* old_tail) {
    auto new_node = new NodeType(match_name, s);

    highwater = s.b;

    add(new_node, old_tail);

    return new_node;
  }

  //----------------------------------------
  // Nodes _must_ be deleted in the reverse order they were allocated.
  // In practice, this means we must delete the "parent" node first and then
  // must delete the child nodes from tail to head.

  void recycle(NodeBase* node) {
    if (node == nullptr) return;

    auto head = node->child_head;
    auto tail = node->child_tail;

    delete node;

    while (tail) {
      auto prev = tail->node_prev;
      recycle(tail);
      tail = prev;
    }
  }

  //----------------------------------------
  // There's one critical detail we need to make the factory work correctly - if
  // we get partway through a match and then fail for some reason, we must
  // "rewind" our match state back to the start of the failed match. This means
  // we must also throw away any parse nodes that were created during the failed
  // match.

  void rewind(cspan s) {
    //printf("rewind\n");
    while (top_tail && (top_tail->span.b > s.a)) {
      auto dead = top_tail;
      top_tail = top_tail->node_prev;
      recycle(dead);
    }
  }

  //----------------------------------------

  size_t node_count() {
    size_t accum = 0;
    for (auto c = top_head; c; c = c->node_next) accum += c->node_count();
    return accum;
  }

  //----------------------------------------

  NodeBase* top_head = nullptr;
  NodeBase* top_tail = nullptr;
  const char* highwater = nullptr;
  int trace_depth = 0;
};

//------------------------------------------------------------------------------
// Matcheroni's default rewind callback does nothing, but if we provide a
// specialized version of it Matcheroni will call it as needed.

template <>
inline void matcheroni::parser_rewind(void* ctx, cspan s) {
  if (ctx) {
    ((Context*)ctx)->rewind(s);
  }
}

//------------------------------------------------------------------------------
// To convert our pattern matches to parse nodes, we create a Factory<>
// matcher that constructs a new NodeType() for a successful match, attaches
// any sub-nodes to it, and places it on a node list.

inline static int capture_count = 0;
inline static int create_count = 0;

template <matcheroni::StringParam match_name, typename pattern,
          typename NodeType>
struct CaptureNamed {
  static cspan match(void* ctx, cspan s) {
    //printf("capture count %d\n", ++capture_count);

    Context* context = (Context*)ctx;
    auto old_tail = context->top_tail;
#ifdef PARSERONI_FAST_MODE
    auto old_state = NodeBase::slabs.save_state();
#endif

    auto end = pattern::match(context, s);

    if (end.is_valid()) {
      //printf("create count %d\n", ++create_count);
      cspan node_span = {s.a, end.a};
      context->create<NodeType>(match_name.str_val, node_span, old_tail);
    }
    else {
      //assert(parser->top_tail == old_tail);
      context->top_tail = old_tail;
#ifdef PARSERONI_FAST_MODE
      NodeBase::slabs.restore_state(old_state);
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
  static cspan match(void* ctx, cspan s) {
    Context* context = (Context*)ctx;

    //----------------------------------------
    // Save tail and match pattern

    auto old_tail = context->top_tail;
#ifdef PARSERONI_FAST_MODE
    auto old_state = NodeBase::slabs.save_state();
#endif

    auto end = matcheroni::Seq<rest...>::match(context, s);

    if (!end.is_valid()) {
      context->top_tail = old_tail;
#ifdef PARSERONI_FAST_MODE
      NodeBase::slabs.restore_state(old_state);
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

    // Resize the bookmark's span and clear its bookmark flag
    c->span.a = s.a;
    c->flags &= ~1;

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

template<matcheroni::StringParam match_name, typename NodeType>
struct CaptureEnd {
  static cspan match(void* ctx, cspan s) {
    //print_indent(); printf("CaptureEnd %s @ %-10.10s\n", match_name.str_val, s.a);
    Context* context = (Context*)ctx;
    cspan new_span(s.a, s.a);
    auto n = context->create<NodeType>(match_name.str_val, new_span, context->top_tail);
    n->flags |= 1;
    return s;
  }
};

//------------------------------------------------------------------------------
// To debug our patterns, we create a Trace<> matcher that prints out a
// diagram of the current match context, the matchers being tried, and
// whether they succeeded.

// Example snippet:

// {(good|bad)\s+[a-z]*$} |  pos_set ?
// {(good|bad)\s+[a-z]*$} |  pos_set X
// {(good|bad)\s+[a-z]*$} |  group ?
// {good|bad)\s+[a-z]*$ } |  |  oneof ?
// {good|bad)\s+[a-z]*$ } |  |  |  text ?
// {good|bad)\s+[a-z]*$ } |  |  |  text OK

// Uncomment this to print a full trace of the regex matching process. Note -
// the trace will be _very_ long, even for small regexes.

template <matcheroni::StringParam match_name, typename P>
struct Trace {
  static cspan match(void* ctx, cspan s) {
    assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    auto parser = (Context*)ctx;
    print_bar(parser->trace_depth++, s, match_name.str_val, "?");
    auto end = P::match(ctx, s);
    print_bar(--parser->trace_depth, s, match_name.str_val,
              end.is_valid() ? "OK" : "X");

    return end;
  }
};

//------------------------------------------------------------------------------
