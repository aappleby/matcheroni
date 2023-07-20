//------------------------------------------------------------------------------
// Parseroni JSON tutorial, part 1A.

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"
#include "matcheroni/dump.hpp"

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
  // clang-format on

  static TextSpan match(TextContext& ctx, TextSpan s) {
    return number::match(ctx, s);
  }
};

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("json_tut1a\n");

  TextContext ctx;
  auto text = to_span("-12345.6789e10 Hello\nWorld\t\r");
  auto tail = JsonParser::match(ctx, text);
  print_summary(text, tail, 40);

  return 0;
}

//------------------------------------------------------------------------------
