//------------------------------------------------------------------------------
// Parseroni JSON tutorial, part 1A.

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
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
  // clang-format on

  static cspan match(void* ctx, cspan s) {
    return number::match(ctx, s);
  }
};

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  auto text = to_span(R"(-123456.678 Hello World)");
  auto tail = JsonParser::match(nullptr, text);

  if (tail.is_valid()) {
    print_match(text, text - tail);
  } else {
    print_fail(text, tail);
  }

  return 0;
}

//------------------------------------------------------------------------------
