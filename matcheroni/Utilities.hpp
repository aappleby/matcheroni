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
namespace utils {

inline void print_span(const char* prefix, TextSpan span) {
  printf("%s", prefix);
  for (auto c = span.begin; c < span.end; c++) putc(*c, stdout);
  putc('\n', stdout);
}

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

  inline static void reset_count() {
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

template <StringParam match_tag, typename P>
struct TraceText {
  template<typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (body.is_empty()) return body.fail();

    auto name = match_tag.str_val;

    //print_bar(ctx.trace_depth++, body, name, "?");
    int depth = ctx.trace_depth++;

    print_match(body.begin, body.end, body.end, 0xCCCCCC, 0xCCCCCC, 50);
    print_trellis(depth, name, "?", 0xCCCCCC);

    auto tail = P::match(ctx, body);
    depth = --ctx.trace_depth;

    if (tail.is_valid()) {
      print_match(body.begin, tail.begin, body.end, 0x80FF80, 0xCCCCCC, 50);
      print_trellis(depth, name, "!", 0x80FF80);
    }
    else {
      print_match(body.begin, tail.end, body.end, 0xCCCCCC, 0x8080FF, 50);
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

inline std::string read(const std::string& path) {
  return read(path.c_str());
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

  for (auto c = node->match_tag; *c; c++) {
    h = (h * 975313579) ^ *c;
  }

  for (auto c = node->span.begin; c < node->span.end; c++) {
    h = (h * 123456789) ^ *c;
  }

  for (auto c = node->child_head; c; c = c->node_next) {
    h = (h * 987654321) ^ hash_tree(c, depth + 1);
  }

  return h;
}

template<typename context>
inline uint64_t hash_context(context& ctx) {
  uint64_t h = 123456789;
  for (auto node = ctx.top_head; node; node = node->node_next) {
    h = (h * 373781549) ^ hash_tree(node);
  }
  return h;
}

//------------------------------------------------------------------------------

constexpr uint32_t pack_color(double r, double g, double b) {
  int ir = int(r * 255.0);
  int ig = int(g * 255.0);
  int ib = int(b * 255.0);

  auto result = (ib << 16) | (ig << 8) | (ir << 0);
  printf("0x%08x\n", result);

  return result;
}

constexpr uint32_t phi_color(int n) {

  uint32_t colors[12] = {
    0x00FF7F7F, // COL_BLUE
    0x00FF7FBF, // COL_PURPLE
    0x00FF7FFF, // COL_MAGENTA
    0x00BF7FFF, // COL_PINK
    0x007F7FFF, // COL_RED
    0x007FBFFF, // COL_ORANGE
    0x007FFFFF, // COL_YELLOW
    0x007FFFBF, // COL_LIME
    0x007FFF7F, // COL_GREEN
    0x00BFFF7F, // COL_MINT
    0x00FFFF7F, // COL_TEAL
    0x00FFBF7F, // COL_SKY
  };

  return colors[n % 12];

#if 0
  //double phi = 1.61803398874989484820;
  //double theta = (phi - 1.0) * n;


  double theta = (1.0 / 12.0) * (n * 1);

  theta = theta - int(theta);
  theta = theta * 6.0;

  //printf("%f\n", theta);

  double s = 0.5;
  double v = 1.0;

  double a = theta - int(theta);
  double b = 1.0;
  double c = 1.0 - a;
  double d = 0.0;

  double max = v;
  double min = (1 - s) * v;

  a = (a * (max - min)) + min;
  b = (b * (max - min)) + min;
  c = (c * (max - min)) + min;
  d = (d * (max - min)) + min;

  //printf("%f %f %f %f\n", a, b, c, d);

  if (theta < 1.0) return pack_color(a, d, b);
  if (theta < 2.0) return pack_color(b, d, c);
  if (theta < 3.0) return pack_color(b, a, d);
  if (theta < 4.0) return pack_color(c, b, d);
  if (theta < 5.0) return pack_color(d, b, a);
  else             return pack_color(d, c, b);
#endif
}

//------------------------------------------------------------------------------
}; // namespace utils
}; // namespace matcheroni
