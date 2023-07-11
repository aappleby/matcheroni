//------------------------------------------------------------------------------
// Parseroni JSON tutorial, part 1B.

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"

using namespace matcheroni;
using cspan = Span<const char>;

//------------------------------------------------------------------------------
// clang-format off

using sign      = Atom<'+', '-'>;
using digit     = Range<'0', '9'>;
using onenine   = Range<'1', '9'>;
using digits    = Some<digit>;
using integer   = Seq<Opt<Atom<'-'>>, Oneof<Seq<onenine, digits>, digit>>;
using fraction  = Seq<Atom<'.'>, digits>;
using exponent  = Seq<Atom<'e', 'E'>, Opt<sign>, digits>;
using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;

using ws        = Any<Atom<' ', '\n', '\r', '\t'>>;
using hex       = Oneof<Range<'0', '9'>, Range<'A', 'F'>, Range<'a', 'f'>>;
using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
using keyword   = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;
using character = Oneof<NotAtom<'"', '\\'>, Seq<Atom<'\\'>, escape>>;
using string    = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

// clang-format on
//------------------------------------------------------------------------------

void put(char c) {
  if (c == '\n')
    putc(' ', stdout);
  else if (c == '\r')
    putc(' ', stdout);
  else if (c == '\t')
    putc(' ', stdout);
  else
    putc(c, stdout);
}

void put_span(cspan s) {
  if (s.a == nullptr)
    printf("<null>");
  else if (s.a == s.b)
    printf("<empty>");
  else
    for (auto c = s.a; c < s.b; c++) put(*c);
}

//------------------------------------------------------------------------------

template <typename pattern>
void test(const char* text) {
  int len = strlen(text);
  cspan all_text(text, text + len);

  for (int start = 0; start < len; start++) {
    cspan span(all_text.a + start, all_text.b);
    auto tail = pattern::match(nullptr, span);
    if (!tail.valid()) continue;

    printf("Match found : `");
    auto match = span - tail;
    put_span(match);
    printf("`\n");

    put_span(all_text);
    printf("\n");

    for (auto j = 0; j < start; j++) put('_');
    for (auto j = 0; j < match.len(); j++) put('^');
    for (auto j = 0; j < tail.len(); j++) put('_');
    printf("\n");
    break;
  }
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  const char* text = R"(Matcher Test 123456.678 "Hello World" Matcher Test)";

  test<hex>(text);

  return 0;
}

//------------------------------------------------------------------------------
