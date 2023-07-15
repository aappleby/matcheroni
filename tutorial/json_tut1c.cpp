//------------------------------------------------------------------------------
// Parseroni JSON tutorial, part 1C.

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

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
  using ws        = Any<Atom<' ', '\n', '\r', '\t'>>;
  using hex       = Oneof<Range<'0', '9'>, Range<'A', 'F'>, Range<'a', 'f'>>;
  using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
  using character = Oneof<NotAtom<'"', '\\'>, Seq<Atom<'\\'>, escape>>;
  using string    = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;
  using keyword   = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;

  template <typename P>
  using list = Opt<Seq<P, Any<Seq<ws, Atom<','>, ws, P>>>>;

  static cspan value(void* ctx, cspan s) {
    return
    Oneof<
      Trace<"array",   array>,
      Trace<"number",  number>,
      Trace<"object",  object>,
      Trace<"string",  string>,
      Trace<"keyword", keyword>
    >::match(ctx, s);
  }

  using array =
  Seq<
    Atom<'['>,
    ws,
    list<Trace<"element", Ref<value>>>,
    ws,
    Atom<']'>
  >;

  using pair =
  Seq<
    Trace<"key", string>,
    ws,
    Atom<':'>,
    ws,
    Trace<"val", Ref<value>>
  >;

  using object =
  Seq<
    Atom<'{'>,
    ws,
    list<Trace<"pair", pair>>,
    ws,
    Atom<'}'>
  >;

  static cspan match(void* ctx, cspan s) {
    return Seq<ws, Ref<value>, ws>::match(ctx, s);
  }
  // clang-format on
};

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  auto text = to_span(R"( { "zarg" : "whop", "foo" : [1,2,3] } )");
  Context<TextNode> context;
  auto tail = JsonParser::match(&context, text);

  if (tail.is_valid()) {
    print_match(text, text - tail);
  } else {
    print_fail(text, tail);
  }

  return 0;
}

//------------------------------------------------------------------------------
