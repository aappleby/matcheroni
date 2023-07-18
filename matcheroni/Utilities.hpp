// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include "matcheroni/Matcheroni.hpp"  // for Span
//#include "matcheroni/Parseroni.hpp"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>    // for exit
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <time.h>      // for clock_gettime, CLOCK_PROCESS_CP...
#include <typeinfo>    // for type_info

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

#if 0
template <StringParam match_name, typename P>
struct Trace {
  template<typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    using ContextType = ContextBase<atom>;
    using SpanType = Span<atom>;
    //printf("match_name %s\n", match_name.str_val);

    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    auto parser = (ContextType*)ctx;
    print_bar(parser->trace_depth++, s, match_name.str_val, "?");
    auto end = P::match(ctx, s);
    print_bar(--parser->trace_depth, s, match_name.str_val,
              end.is_valid() ? "OK" : "X");

    return end;
  }
};
#endif

#if 0
inline static int indent_depth = 0;

//------------------------------------------------------------------------------

template<typename NodeType, typename atom>
void print_trace_start(atom* a) {
  static constexpr int context_len = 60;
  printf("[");
  print_escaped(a->get_lex_debug()->span_a, context_len, 0x404040);
  printf("] ");
  for (int i = 0; i < indent_depth; i++) printf("|   ");
  print_class_name<NodeType>();
  printf("?\n");
  indent_depth += 1;
}

//------------------------------------------------------------------------------

template<typename NodeType, typename atom>
void print_trace_end(atom* a, atom* end) {
  static constexpr int context_len = 60;
  indent_depth -= 1;
  printf("[");
  if (end) {
    int match_len = end->get_lex_debug()->span_a - a->get_lex_debug()->span_a;
    if (match_len > context_len) match_len = context_len;
    int left_len = context_len - match_len;
    print_escaped(a->get_lex_debug()->span_a, match_len,  0x60C000);
    print_escaped(end->get_lex_debug()->span_a, left_len, 0x404040);
  }
  else {
    print_escaped(a->get_lex_debug()->span_a, context_len, 0x2020A0);
  }
  printf("] ");
  for (int i = 0; i < indent_depth; i++) printf("|   ");
  print_class_name<NodeType>();
  printf(end ? " OK\n" : " XXX\n");

#ifdef EXTRA_DEBUG
  if (end) {
    printf("\n");
    print_class_name<NodeType>();
    printf("\n");

    for (auto c = a; c < end; c++) {
      c->dump_token();
    }
    printf("\n");
  }
#endif
}

//------------------------------------------------------------------------------

template<typename NodeType>
struct Trace {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    print_trace_start<NodeType, atom>(a);
    auto end = NodeType::match(ctx, a, b);
    print_trace_end<NodeType, atom>(a, end);
    return end;
  }
};

//------------------------------------------------------------------------------
#endif

//------------------------------------------------------------------------------

inline TextSpan to_span(const char* text) {
  return TextSpan(text, text + strlen(text));
}

inline TextSpan to_span(const std::string& s) {
  return TextSpan(s.data(), s.data() + s.size());
}

inline std::string to_string(TextSpan s) {
  matcheroni_assert(s.a);
  return std::string(s.a, s.b);
}

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
    n = (NodeType*)n->node_next();
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
    n = (NodeType*)n->node_next();
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

inline void set_color(uint32_t c) {
  if (c) {
    printf("\u001b[38;2;%d;%d;%dm", (c >> 0) & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF);
  }
  else {
    printf("\u001b[0m");
  }
}

//------------------------------------------------------------------------------

inline void print_flat(TextSpan s, size_t max_len = 0) {
  if (max_len == 0) max_len = s.len();

  //printf("%p %p len %ld\n", s.a, s.b, s.len());

  if (!s.is_valid()) {
    printf("<invalid>");
    for (size_t i = 0; i < max_len - strlen("<invalid>"); i++) putc(' ', stdout);
    return;
  }

  if (s.is_empty()) {
    printf("<empty>");
    for (size_t i = 0; i < max_len - strlen("<empty>"); i++) putc(' ', stdout);
    return;
  }

  size_t span_len = max_len;
  if (s.len() > max_len) span_len -= 3;

  for (size_t i = 0; i < span_len; i++) {
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

inline void print_bar(int depth, TextSpan s, const char* val, const char* suffix) {
  set_color(0);
  printf("{");
  set_color(0xAAFFAA);
  print_flat(s, 40);
  set_color(0);
  printf("}");

  set_color(0x404040);
  printf(depth == 0 ? " *" : "  ");
  for (int i = 0; i < depth; i++) {
    printf(i == depth - 1 ? "|-" : "| ");
  }

  set_color(0xFFAAAA);
  printf("%s %s", val, suffix);
  printf("\n");

  set_color(0);
}

//------------------------------------------------------------------------------

inline void print_match(TextSpan text, TextSpan match) {
  printf("Match found:\n");
  print_flat(text);
  printf("\n");

  size_t prefix_len = match.a - text.a;
  size_t suffix_len = text.b - match.b;

  set_color(0x404040);
  for (size_t i = 0; i < prefix_len;  i++) putc('_', stdout);

  set_color(0x80FF80);
  for (size_t i = 0; i < match.len(); i++) putc('^', stdout);

  set_color(0x404040);
  for (size_t i = 0; i < suffix_len;  i++) putc('_', stdout);

  printf("\n");
  set_color(0);
}

//------------------------------------------------------------------------------

inline void print_fail(TextSpan text, TextSpan tail) {
  printf("Match failed here:\n");
  print_flat(text);
  printf("\n");

  size_t fail_pos = tail.b - text.a;
  size_t suffix_len = text.b - tail.b;

  set_color(0x404040);
  for (size_t i = 0; i < fail_pos;   i++) putc('_', stdout);

  set_color(0x8080FF);
  for (size_t i = 0; i < suffix_len; i++) putc('^', stdout);

  printf("\n");
  set_color(0);
}

//------------------------------------------------------------------------------

inline void print_escaped(const char* s, int len, unsigned int color) {
  if (len < 0) {
    exit(1);
  }
  set_color(color);
  while(len) {
    auto c = *s++;
    if      (c == 0)    break;
    else if (c == ' ')  putc(' ', stdout);
    else if (c == '\n') putc(' ', stdout);
    else if (c == '\r') putc(' ', stdout);
    else if (c == '\t') putc(' ', stdout);
    else                putc(c,   stdout);
    len--;
  }
  set_color(0);

  return;
}

//------------------------------------------------------------------------------
// Prints a text representation of the parse tree.

template<typename NodeType>
inline void print_tree(const NodeType* node, int depth = 0) {
  print_bar(depth, node->span, node->match_name, "");
  for (auto c = node->child_head(); c; c = c->node_next()) {
    print_tree(c, depth + 1);
  }
}

template<typename context>
inline void print_context(const context& ctx, int depth = 0) {
  for (auto node = ctx.top_head(); node; node = node->node_next()) {
    print_tree(node);
  }
}

//------------------------------------------------------------------------------

/*
inline void print_typeid_name(const char* name, int max_len = 0) {
  int name_len = 0;

  while((*name >= '0') && (*name <= '9')) {
    name_len *= 10;
    name_len += *name - '0';
    name++;
  }

  if (max_len && name_len > max_len) name_len = max_len;

  for (int i = 0; i < name_len; i++) {
    putc(name[i], stdout);
  }
  for (int i = name_len; i < max_len; i++) {
    putc(' ', stdout);
  }
}
*/

//------------------------------------------------------------------------------

/*
template<typename T>
inline void print_class_name(int max_len = 0) {
  print_typeid_name(typeid(T).name(), max_len);
}
*/

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

template<typename NodeType>
inline uint64_t hash_tree(const NodeType* node, int depth = 0) {
  uint64_t h = 1 + depth * 0x87654321;

  for (auto c = node->match_name; *c; c++) {
    h = (h * 975313579) ^ *c;
  }

  for (auto c = node->span.a; c < node->span.b; c++) {
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
