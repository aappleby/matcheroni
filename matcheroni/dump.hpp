#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>  // for exit
#include <string.h>

#include "matcheroni/Matcheroni.hpp"  // for Span

namespace matcheroni {
namespace utils {

//------------------------------------------------------------------------------

inline uint32_t dim_color(uint32_t color) {
  int r = (color >>  0) & 0xFF;
  int g = (color >>  8) & 0xFF;
  int b = (color >> 16) & 0xFF;

  r = (r * 2) / 5;
  g = (g * 2) / 5;
  b = (b * 2) / 5;

  return (r << 0) | (g << 8) | (b << 16);
}

inline void set_color(uint32_t c) {
  static uint32_t current_color = 0;
  if (current_color == c) return;
  current_color = c;
  if (c) {
    printf("\u001b[38;2;%d;%d;%dm", (c >> 0) & 0xFF, (c >> 8) & 0xFF,
           (c >> 16) & 0xFF);
  } else {
    printf("\u001b[0m");
  }
}

//------------------------------------------------------------------------------

struct SpanDumper {
  size_t len = 0;

  void put(char c, uint32_t color) {
    if (color == 0) color = 0xCCCCCC;
    if      (c == ' ' ) { set_color(dim_color(color)); putc('_', stdout); }
    else if (c == '\n') { set_color(dim_color(color)); putc('n', stdout); }
    else if (c == '\r') { set_color(dim_color(color)); putc('r', stdout); }
    else if (c == '\t') { set_color(dim_color(color)); putc('t', stdout); }
    else                { set_color(color);            putc(c,   stdout); }
    len++;
    fflush(stdout);
  }

  void put(const char* s, uint32_t color = 0) {
    for (auto c = s; *c; c++) put(*c, color);
  }

  void put(const char* a, const char* b, uint32_t color = 0) {
    for (auto c = a; c < b; c++) put(*c, color);
  }

  void put(TextSpan body, uint32_t color = 0) {
    for (auto c = body.begin; c < body.end; c++) put(*c, color);
  }

  void reset() {
    set_color(0);
    len = 0;
  }
};

//------------------------------------------------------------------------------

inline void print_trellis(int depth, const char* name, const char* suffix,
                          uint32_t color) {
  set_color(0x808080);
  printf(depth == 0 ? " *" : "  ");
  for (int i = 0; i < depth; i++) {
    printf(i == depth - 1 ? "|--" : "|  ");
  }
  set_color(color);
  printf("%s %s\n", name, suffix);
  set_color(0);
  fflush(stdout);
}

//------------------------------------------------------------------------------

inline void print_match(TextSpan span_a, TextSpan span_b,
                        uint32_t color_a, uint32_t color_b, size_t width) {
  SpanDumper d;

  for (auto cursor = span_a.begin; cursor < span_a.end && d.len < width; cursor++) {
    d.put(*cursor, color_a);
  }

  for (auto cursor = span_b.begin; cursor < span_b.end && d.len < width; cursor++) {
    d.put(*cursor, color_b);
  }

  while (d.len < width) d.put('@', dim_color(color_b));

  set_color(0);
  fflush(stdout);
}

//------------------------------------------------------------------------------

inline void print_match(TextSpan span, TextSpan text, size_t width) {
  if (span.is_valid()) {
    auto head = span;
    auto tail = TextSpan(span.end, text.end);
    print_match(head, tail, 0x80FF80, 0xCCCCCC, width);
  } else {
    auto head = TextSpan(text.begin, span.end);
    auto tail = TextSpan(span.end, text.end);
    print_match(head, tail, 0xCCCCCC, 0x8080FF, width);
  }
}

//------------------------------------------------------------------------------
// Prints a text representation of the parse tree.

template<typename node_type>
inline void print_tree(TextSpan text, const node_type* node, int width, int depth) {
  print_match(node->as_text(), text, 0x80FF80, 0xCCCCCC, width);
  print_trellis(depth, node->match_name, "", 0xFFAAAA);

  for (auto c = node->child_head; c; c = c->node_next) {
    print_tree(text, c, width, depth + 1);
  }
}

template<typename context>
inline void print_context(TextSpan text, const context& ctx, int width) {
  for (auto node = ctx.top_head; node; node = node->node_next) {
    print_tree(text, node, width, 0);
  }
}

inline void print_summary(TextSpan text, TextSpan tail, int width) {
  printf("Match text:\n");
  print_match(text, text, width);
  printf("\n\n");
  printf("Match result:\n");
  print_match(text, tail, width);
  printf("\n\n");
}

template<typename context>
inline void print_summary(TextSpan text, TextSpan tail, const context& ctx, int width) {
  print_summary(text, tail, width);
  printf("Parse tree:\n");
  print_context(text, ctx, width);
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

};  // namespace utils
};  // namespace matcheroni

//------------------------------------------------------------------------------

#if 0
void CLexer::dump_lexemes() {
  for (auto& l : tokens) {
    l.dump();
    printf("\n");
  }
}
#endif

//----------------------------------------------------------------------------

#if 0
void CToken::dump() const {
  const int span_len = 20;
  std::string dump = "";

  if (type == LEX_BOF) dump = "<bof>";
  if (type == LEX_EOF) dump = "<eof>";

  for (auto c = a; c < b; c++) {
    if      (*c == '\n') dump += "\\n";
    else if (*c == '\t') dump += "\\t";
    else if (*c == '\r') dump += "\\r";
    else                 dump += *c;
    if (dump.size() >= span_len) break;
  }

  dump = '`' + dump + '`';
  if (dump.size() > span_len) {
    dump.resize(span_len - 4);
    dump = dump + "...`";
  }
  while (dump.size() < span_len) dump += " ";

  set_color(type_to_color());
  printf("%-14.14s ", type_to_str());
  set_color(0);
  printf("%s", dump.c_str());
  set_color(0);
}
#endif

//------------------------------------------------------------------------------

#if 0
inline void print_bar2(int depth, TextSpan body, const char* val, const char* suffix) {
  set_color(0);
  printf("{");
  set_color(0xAAFFAA);
  print_flat(s, TextSpan(body.b, body.b), 40);
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

inline void print_tree2(const CNode* node, int depth = 0) {
  TextSpan text(node->span.a->a, node->span.b->a);

  print_bar2(depth, text, node->match_name, "");
  for (auto c = node->child_head; c; c = c->node_next) {
    print_tree2(c, depth + 1);
  }
}
#endif

#if 0
void CNode::dump_tree(int max_depth, int indentation) {
  print_tree2(this);
  /*
  const CNode* n = this;
  if (max_depth && indentation == max_depth) return;

  printf("%s %p {%p-%p} ", n->match_name, n, n->span.a, n->span.b);
  printf("\n");
  */

  /*
  printf("{%-40.40s}", escape_span(n).c_str());

  for (int i = 0; i < indentation; i++) printf(" | ");

  if (n->precedence) {
    printf("[%02d %2d] ",
      n->precedence,
      n->assoc
      //n->assoc > 0 ? '>' : n->assoc < 0 ? '<' : '-'
    );
  }
  else {
    printf("[-O-] ");
  }

  if (n->tok_a()) set_color(n->tok_a()->type_to_color());
  //if (!field.empty()) printf("%-10.10s : ", field.c_str());

  n->print_class_name(20);
  set_color(0);
  printf("\n");

  for (auto c = n->head; c; c = c->next) {
    c->dump_tree(max_depth, indentation + 1);
  }
  */
}

#endif

//------------------------------------------------------------------------------
