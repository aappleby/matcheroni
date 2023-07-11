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

cspan match_value(void* ctx, cspan s);
using value = Ref<match_value>;

using pair =
Seq<
  string,
  ws,
  Atom<':'>,
  ws,
  value
>;

template<typename P>
using comma_separated = Seq<P, Any<Seq<ws, Atom<','>, ws, P>>>;

using object =
Seq<
  Atom<'{'>, ws,
  Opt<comma_separated<pair>>, ws,
  Atom<'}'>
>;

using array =
Seq<
  Atom<'['>,
  ws,
  Opt<comma_separated<value>>, ws,
  Atom<']'>
>;

cspan match_value(void* ctx, cspan s) {
  using value =
  Oneof<
    array,
    number,
    object,
    string,
    keyword
  >;
  return value::match(ctx, s);
}

using json = Seq<ws, value, ws>;

/*
seq json
  oneof value
    seq object
      seq pair
        seq string ws atom ws value
*/

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
  text = "aaaaaabbbbb";

  int len = strlen(text);
  cspan all_text(text, text + len);

  //auto tail = pattern::match(nullptr, all_text);

  using pattern2 =
  Seq<
    Some<Atom<'a'>>,
    Not<Atom<'b'>>
  >;

  auto tail = pattern2::match(nullptr, all_text);

  if (tail.valid()) {
    printf("Match found: `");
    auto match = all_text - tail;
    put_span(match);
    printf("`\n");

    put_span(all_text);
    printf("\n");

    for (auto j = 0; j < match.len(); j++) put('^');
    for (auto j = 0; j < tail.len(); j++) put('_');
    printf("\n");
  }
  else {
    printf("Match failed here:\n");
    put_span(all_text);
    printf("\n");
    int fail_pos = tail.b - all_text.a;
    for (int i = 0; i < fail_pos; i++) put('_');
    for (int i = fail_pos; i < len; i++) put('^');
    printf("\n");
  }
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  const char* text = R"( { "foo" : z"bar" } )";

  test<json>(text);

  return 0;
}

//------------------------------------------------------------------------------
