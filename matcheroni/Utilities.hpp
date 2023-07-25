// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include "matcheroni/Matcheroni.hpp"  // for Span

#include "matcheroni/dump.hpp"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>    // for exit
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <time.h>      // for clock_gettime, CLOCK_PROCESS_CP...
#include <typeinfo>    // for type_info
#include <vector>

#include <string_view>

namespace matcheroni {

//------------------------------------------------------------------------------

template<typename T>
struct InstanceCounter {
  InstanceCounter() {
    live++;
  }

  ~InstanceCounter() {
    live--;
    dead++;
  }

  inline static void reset() {
    live = 0;
    dead = 0;
  }

  inline static size_t live = 0;
  inline static size_t dead = 0;
};

//------------------------------------------------------------------------------
// To debug our patterns, we create a TraceText<> matcher that prints out a
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

template <StringParam match_name, typename P>
struct TraceText {
  template<typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (body.is_empty()) return body.fail();

    auto name = match_name.str_val;

    //print_bar(ctx.trace_depth++, body, name, "?");
    int depth = ctx.trace_depth++;

    print_match(body, body, 50);
    print_trellis(depth, name, "?", 0xCCCCCC);

    auto tail = P::match(ctx, body);
    depth = --ctx.trace_depth;

    print_match(body, tail, 50);
    if (tail.is_valid()) {
      print_trellis(depth, name, "!", 0x80FF80);
    }
    else {
      print_trellis(depth, name, "X", 0x8080FF);
    }

    return tail;
  }
};

//------------------------------------------------------------------------------

inline TextSpan to_span(const char* text) {
  return TextSpan(text, text + strlen(text));
}

inline TextSpan to_span(const std::string& text) {
  return TextSpan(text.data(), text.data() + text.size());
}

template<typename atom>
inline Span<atom> to_span(const std::vector<atom>& atoms) {
  return Span<atom>(atoms.data(), atoms.data() + atoms.size());
}

//------------------------------------------------------------------------------

inline std::string to_string(TextSpan body) {
  matcheroni_assert(body.begin);
  return std::string(body.begin, body.end);
}

//------------------------------------------------------------------------------

inline bool operator==(TextSpan a, const std::string& b) {
  return a.is_valid() && to_string(a) == b;
}

inline bool operator==(const std::string& a, TextSpan b) {
  return b.is_valid() && a == to_string(b);
}

inline bool operator!=(TextSpan a, const std::string& b) {
  return !(a == b);
}

inline bool operator!=(const std::string& a, TextSpan b) {
  return !(a == b);
}

//------------------------------------------------------------------------------

#if 0
template<typename NodeType>
struct NodeIterator {
  NodeIterator(NodeType* cursor) : n(cursor) {}
  NodeIterator& operator++() {
    n = n->node_next();
    return *this;
  }
  bool operator!=(NodeIterator& b) const { return n != b.n; }
  NodeType* operator*() const { return n; }
  NodeType* n;
};

template<typename NodeType>
inline NodeIterator<NodeType> begin(NodeType* parent) {
  return NodeIterator<NodeType>(parent->child_head());
}

template<typename NodeType>
inline NodeIterator<NodeType> end(NodeType* parent) {
  return NodeIterator<NodeType>(nullptr);
}

template<typename NodeType>
struct ConstNodeIterator {
  ConstNodeIterator(const NodeType* cursor) : n(cursor) {}
  ConstNodeIterator& operator++() {
    n = n->node_next();
    return *this;
  }
  bool operator!=(const ConstNodeIterator& b) const { return n != b.n; }
  const NodeType* operator*() const { return n; }
  const NodeType* n;
};

template<typename NodeType>
inline ConstNodeIterator<NodeType> begin(const NodeType* parent) {
  return ConstNodeIterator<NodeType>(parent->child_head());
}

template<typename NodeType>
inline ConstNodeIterator<NodeType> end(const NodeType* parent) {
  return ConstNodeIterator<NodeType>(nullptr);
}
#endif


//------------------------------------------------------------------------------

inline double timestamp_ms() {
  timespec t;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
  return double(t.tv_sec) * 1e3 + double(t.tv_nsec) * 1e-6;
}

//------------------------------------------------------------------------------

inline std::string read(const char* path) {
  struct stat statbuf;
  if (stat(path, &statbuf) == -1) return "";

  std::string buf;
  buf.resize(statbuf.st_size);

  FILE* f = fopen(path, "rb");
  auto size = fread(buf.data(), statbuf.st_size, 1, f);
  (void)size;
  fclose(f);

  return buf;
}

inline void read(const char* path, std::string& text) {
  struct stat statbuf;
  if (stat(path, &statbuf) == -1) return;

  text.resize(statbuf.st_size);

  FILE* f = fopen(path, "rb");
  auto size = fread(text.data(), statbuf.st_size, 1, f);
  (void)size;
  fclose(f);
}

inline void read(const char* path, char*& text_out, size_t& size_out) {
  struct stat statbuf;
  if (stat(path, &statbuf) == -1) return;

  text_out = new char[statbuf.st_size];
  size_out = statbuf.st_size;

  FILE* f = fopen(path, "rb");
  auto size = fread(text_out, statbuf.st_size, 1, f);
  (void)size;
  fclose(f);
}

//------------------------------------------------------------------------------

template<typename node_type>
inline uint64_t hash_tree(const node_type* node, int depth = 0) {
  uint64_t h = 1 + depth * 0x87654321;

  for (auto c = node->match_name; *c; c++) {
    h = (h * 975313579) ^ *c;
  }

  for (auto c = node->span.begin; c < node->span.end; c++) {
    h = (h * 123456789) ^ *c;
  }

  for (auto c = node->child_head(); c; c = c->node_next()) {
    h = (h * 987654321) ^ hash_tree(c, depth + 1);
  }

  return h;
}

template<typename context>
inline uint64_t hash_context(context& ctx) {
  uint64_t h = 123456789;
  for (auto node = ctx.top_head(); node; node = node->node_next()) {
    h = (h * 373781549) ^ hash_tree(node);
  }
  return h;
}

//------------------------------------------------------------------------------

}; // namespace matcheroni
