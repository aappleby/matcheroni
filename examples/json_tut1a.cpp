//------------------------------------------------------------------------------
// Parseroni JSON tutorial, part 1A.

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace matcheroni;
using cspan = Span<const char>;

//------------------------------------------------------------------------------

using sign      = Atom<'+','-'>;
using digit     = Range<'0','9'>;
using onenine   = Range<'1','9'>;
using digits    = Some<digit>;
using integer   = Seq< Opt<Atom<'-'>>, Oneof<Seq<onenine,digits>,digit> >;
using fraction  = Seq<Atom<'.'>, digits>;
using exponent  = Seq<Atom<'e','E'>, Opt<sign>, digits>;
using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;

//------------------------------------------------------------------------------

template<typename pattern>
void test(const char* text) {
  cspan span(text, text + strlen(text));
  auto tail = pattern::match(nullptr, span);

  printf("%s\n", text);

  for (auto c = span.a; c < span.b; c++) {
    printf("%c", c < tail.a ? '^' : '_');
  }
  printf("\n");
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {

  const char* text = "A-123456.678 Hello World";

  test<number>(text);

  return 0;
}

//------------------------------------------------------------------------------
