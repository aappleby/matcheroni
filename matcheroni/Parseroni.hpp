#pragma once
#include "matcheroni/Matcheroni.hpp"

#include <stdlib.h>

constexpr bool verbose       = false;
constexpr bool count_nodes   = true;
constexpr bool recycle_nodes = true;
constexpr bool dump_tree     = false;

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
    if (verbose) {
      //printf("add_slab %d\n", ++count);
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
    if (recycle_nodes) {
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
  }

  Slab* top_slab;
  int   current_size;
  int   max_size;
};

//------------------------------------------------------------------------------

struct NodeBase {

  virtual ~NodeBase() {}

  //----------------------------------------

  void init(const char* a, const char* b, NodeBase* child_head, NodeBase* child_tail) {
    this->a = a;
    this->b = b;

    head = child_head;
    tail = child_tail;
    prev = nullptr;
    next = nullptr;

    if (child_head) {
      if (child_head->prev) child_head->prev->next = nullptr;
      child_head->prev = nullptr;
    }
  }

  //----------------------------------------

  size_t node_count() {
    size_t accum = 1;
    for (auto c = head; c; c = c->next) accum += c->node_count();
    return accum;
  }

  //----------------------------------------

  inline static size_t constructor_calls = 0;
  inline static size_t destructor_calls = 0;

  const char* a;
  const char* b;

  NodeBase* prev;
  NodeBase* next;

  NodeBase* head;
  NodeBase* tail;
};

//------------------------------------------------------------------------------
