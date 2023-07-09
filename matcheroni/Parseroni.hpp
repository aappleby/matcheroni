#pragma once
#include "matcheroni/Matcheroni.hpp"

#define PARSERONI_USE_STDIO

#ifdef PARSERONI_USE_STDIO
#include <stdio.h>
#endif

#include <stdlib.h> // for malloc/free

//#define TRACE
//#define FORCE_REWINDS

//------------------------------------------------------------------------------

struct SlabAlloc {

  struct Slab {
    Slab*   prev;
    Slab*   next;
    int     cursor;
    int     highwater;
    char    buf[];
  };

  // slab size is 1 hugepage. seems to work ok.
  static constexpr int header_size = sizeof(Slab);
  static constexpr int slab_size = 2*1024*1024 - header_size;

  SlabAlloc() {
    add_slab();
  }

  void destroy() {
    reset();
    auto c = top_slab;
    while(c) {
      auto next = c->next;
      ::free((void*)c);
      c = next;
    }
    top_slab = nullptr;
  }

  void reset() {
    while(top_slab->prev) top_slab = top_slab->prev;
    for (auto c = top_slab; c; c = c->next) {
      c->cursor = 0;
    }
    current_size = 0;
  }

  void add_slab() {
    if (top_slab && top_slab->next) {
      top_slab = top_slab->next;
      //DCHECK(top_slab->cursor == 0);
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
    //DCHECK(offset + size == top_slab->cursor);
    top_slab->cursor -= size;
    if (top_slab->cursor == 0) {
      if (top_slab->prev) {
        top_slab = top_slab->prev;
      }
    }
    current_size -= size;
    //printf("top_slab->cursor = %ld\n", top_slab->cursor);
  }

  Slab* top_slab;
  int   current_size;
  int   max_size;
};

//------------------------------------------------------------------------------

struct NodeBase {

  NodeBase(const char* a, const char* b) : match_a(a), match_b(b) {
    constructor_calls++;
  }

  virtual ~NodeBase() {
    destructor_calls++;
  }

  static void* operator new(size_t s)               { return slabs.alloc(s); }
  static void* operator new[](size_t s)             { return slabs.alloc(s); }
  static void  operator delete(void* p, size_t s)   { slabs.free(p, s); }
  static void  operator delete[](void* p, size_t s) { slabs.free(p, s); }

  //----------------------------------------

  size_t node_count() {
    size_t accum = 1;
    for (auto c = child_head; c; c = c->node_next) accum += c->node_count();
    return accum;
  }

  //----------------------------------------

  inline static SlabAlloc slabs;
  inline static size_t constructor_calls = 0;
  inline static size_t destructor_calls = 0;

  const char* match_a;
  const char* match_b;

  NodeBase* node_prev;
  NodeBase* node_next;

  NodeBase* child_head;
  NodeBase* child_tail;
};

//------------------------------------------------------------------------------

struct Parser {

  Parser() {}

  virtual ~Parser() {
    reset();
    NodeBase::slabs.destroy();
    //delete [] text;
  }

  void reset() {
    //DCHECK(NodeBase::constructor_calls == NodeBase::destructor_calls);
    //DCHECK(slabs.current_size == 0);

    //delete [] text;
    //text = nullptr;
    //text_size = 0;

    auto c = top_tail;
    while (c) {
      auto prev = c->node_prev;
      recycle(c);
      c = prev;
    }

    top_head = nullptr;
    top_tail = nullptr;

    NodeBase::slabs.reset();
    NodeBase::constructor_calls = 0;
    NodeBase::destructor_calls = 0;
  }

  //----------------------------------------

  void append(NodeBase* new_node) {
    new_node->node_prev  = nullptr;
    new_node->node_next  = nullptr;
    new_node->child_head = nullptr;
    new_node->child_tail = nullptr;

    if (top_tail) {
      new_node->node_prev = top_tail;
      top_tail->node_next = new_node;
      top_tail = new_node;
    }
    else {
      top_head = new_node;
      top_tail = new_node;
    }
  }

  //----------------------------------------

  void splice(NodeBase* new_node, NodeBase* child_head, NodeBase* child_tail) {
    new_node->node_prev  = child_head->node_prev;
    new_node->node_next  = child_tail->node_next;
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
  // Move all nodes in (old_tail,new_tail] to be children of new_node and
  // append new_node to the node list.

  void enclose(NodeBase* new_node, NodeBase* old_tail) {
    if (old_tail == top_tail) {
      append(new_node);
    }
    else {
      auto child_head = old_tail ? old_tail->node_next : top_head;
      auto child_tail = top_tail;
      splice(new_node, child_head, child_tail);
    }
  }

  //----------------------------------------
  // Nodes _must_ be deleted in the reverse order they were allocated.
  // In practice, this means we must delete the "parent" node first and then
  // must delete the child nodes from tail to head.

  void recycle(NodeBase* n) {
    if (n == nullptr) return;

    auto old_tail = n->child_tail;
    delete n;

    auto c = old_tail;
    while(c) {
      auto prev = c->node_prev;
      recycle(c);
      c = prev;
    }
  }

  //----------------------------------------
  // There's one critical detail we need to make the factory work correctly - if
  // we get partway through a match and then fail for some reason, we must
  // "rewind" our match state back to the start of the failed match. This means
  // we must also throw away any parse nodes that were created during the failed
  // match.

  void rewind(const char* a) {
    while(top_tail && top_tail->match_b > a) {
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

  //const char* text = nullptr;
  //int text_size = 0;
  NodeBase* top_head = nullptr;
  NodeBase* top_tail = nullptr;
};

//------------------------------------------------------------------------------
// Matcheroni's default rewind callback does nothing, but if we provide a
// specialized version of it Matcheroni will call it as needed.

template<>
inline void matcheroni::parser_rewind(void* ctx, const char* a, const char* b) {
  ((Parser*)ctx)->rewind(a);
}

//------------------------------------------------------------------------------
// To convert our pattern matches to parse nodes, we create a Factory<>
// matcher that constructs a new NodeType() for a successful match, attaches
// any sub-nodes to it, and places it on a node list.

template<matcheroni::StringParam matcher_name, typename pattern, typename NodeType>
struct CaptureNode {

  static const char* match2(void* ctx, const char* a, const char* b) {
    Parser* parser = (Parser*)ctx;

    auto old_tail = parser->top_tail;
    auto end = pattern::match(parser, a, b);

    if (end) {
      auto new_node = new NodeType(a, b);
      parser->enclose(new_node, old_tail);
    }

    return end;
  }

  static const char* match(void* ctx, const char* a, const char* b) {
    auto end = match2(ctx, a, b);
    if (end) {
      Parser* parser = (Parser*)ctx;
      NodeType* new_node = (NodeType*)parser->top_tail;
      new_node->matcher_name = matcher_name.str_val;
    }

    return end;
  }
};

//------------------------------------------------------------------------------
// Prints a fixed-width span of characters from the source span with all
// whitespace replaced with ' ' and a '@' at EOF.

#ifdef PARSERONI_USE_STDIO

void print_flat(const char* a, const char* b, int max_len) {
  int len = b - a;
  int span_len = max_len;
  if (len > max_len) span_len -= 3;

  for (int i = 0; i < span_len; i++) {
    if      (a + i == b)   putc('@',  stdout);
    if      (a + i >  b)   putc(' ',  stdout);
    else if (a[i] == '\n') putc(' ',  stdout);
    else if (a[i] == '\r') putc(' ',  stdout);
    else if (a[i] == '\t') putc(' ',  stdout);
    else                   putc(a[i], stdout);
  }

  if (len > max_len) printf("...");
}

#endif

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

#ifdef PARSERONI_USE_STDIO

template<matcheroni::StringParam matcher_name, typename P>
struct Trace {

  inline static int trace_depth = 0;

  void print_bar(const char* a, const char* b, const char* val, const char* suffix) {
    printf("|");
    print_flat(a, b, 20);
    printf("| ");
    for (int i = 0; i < trace_depth; i++) printf("|  ");
    printf("%s %s\n", val, suffix);
  }

  template<typename atom>
  atom* trace(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    print_bar(a, b, matcher_name.str_val, "?");
    trace_depth++;
    auto end = P::match(this, a, b);
    trace_depth--;
    print_bar(a, b, matcher_name.str_val, end ? "OK" : "X");
    return end;
  }
};

#endif

//------------------------------------------------------------------------------
