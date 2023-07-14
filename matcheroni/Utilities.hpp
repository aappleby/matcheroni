// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include "matcheroni/Parseroni.hpp"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>    // for exit
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <typeinfo>    // for type_info

namespace matcheroni {

using cspan = matcheroni::Span<const char>;

//------------------------------------------------------------------------------

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

inline void set_color(uint32_t c) {
  if (c) {
    printf("\u001b[38;2;%d;%d;%dm", (c >> 0) & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF);
  }
  else {
    printf("\u001b[0m");
  }
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
  while(len--) putc('#', stdout);
  set_color(0);

  return;
}

//------------------------------------------------------------------------------

inline void print_typeid_name(const char* name) {
  int name_len = 0;
  if (sscanf(name, "%d", &name_len)) {
    while((*name >= '0') && (*name <= '9')) name++;
    for (int i = 0; i < name_len; i++) {
      putc(name[i], stdout);
    }
  }
  else {
    printf("%s", name);
  }
}

//------------------------------------------------------------------------------

template<typename T>
inline void print_class_name() {
  print_typeid_name(typeid(T).name());
}

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
  auto _ = fread(buf.data(), statbuf.st_size, 1, f);
  fclose(f);

  return buf;
}

inline void read(const char* path, std::string& text) {
  struct stat statbuf;
  if (stat(path, &statbuf) == -1) return;

  text.resize(statbuf.st_size);

  FILE* f = fopen(path, "rb");
  auto _ = fread(text.data(), statbuf.st_size, 1, f);
  fclose(f);
}

inline void read(const char* path, char*& text_out, int& size_out) {
  struct stat statbuf;
  if (stat(path, &statbuf) == -1) return;

  text_out = new char[statbuf.st_size];
  size_out = statbuf.st_size;

  FILE* f = fopen(path, "rb");
  auto _ = fread(text_out, size_out, 1, f);
  fclose(f);
}

//------------------------------------------------------------------------------

}; // namespace matcheroni
