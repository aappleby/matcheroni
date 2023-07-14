//------------------------------------------------------------------------------
// Parseroni JSON tutorial, part 1B.

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

//------------------------------------------------------------------------------

struct JsonParser {
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
  using character = Oneof<NotAtom<'"', '\\'>, Seq<Atom<'\\'>, escape>>;
  using string    = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;
  using keyword   = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;
  // clang-format on

  static cspan match(void* ctx, cspan s) {
    return Oneof<number, string, keyword>::match(ctx, s);
  }
};

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  auto text = to_span(R"("Hello World")");
  auto tail = JsonParser::match(nullptr, text);

  if (tail.is_valid()) {
    print_match(text, text - tail);
  } else {
    print_fail(text, tail);
  }

  return 0;
}

//------------------------------------------------------------------------------
