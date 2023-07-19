//------------------------------------------------------------------------------
// Parseroni JSON tutorial, part 1C.

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
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
  using ws        = Any<Atom<' ', '\n', '\r', '\t'>>;
  using hex       = Range<'0','9','a','f','A','F'>;
  using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
  using character = Oneof<NotAtom<'"', '\\'>, Seq<Atom<'\\'>, escape>>;
  using string    = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;
  using keyword   = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;

  template <typename P>
  using list = Opt<Seq<P, Any<Seq<ws, Atom<','>, ws, P>>>>;

  static TextSpan value(TextContext& ctx, TextSpan s) {
    return
    Oneof<
      TraceText<"array",   array>,
      TraceText<"number",  number>,
      TraceText<"object",  object>,
      TraceText<"string",  string>,
      TraceText<"keyword", keyword>
    >::match(ctx, s);
  }

  using array =
  Seq<
    Atom<'['>,
    ws,
    list<TraceText<"element", Ref<value>>>,
    ws,
    Atom<']'>
  >;

  using pair =
  Seq<
    TraceText<"key", string>,
    ws,
    Atom<':'>,
    ws,
    TraceText<"val", Ref<value>>
  >;

  using object =
  Seq<
    Atom<'{'>,
    ws,
    list<TraceText<"pair", pair>>,
    ws,
    Atom<'}'>
  >;

  static TextSpan match(TextContext& ctx, TextSpan s) {
    return Seq<ws, Ref<value>, ws>::match(ctx, s);
  }
  // clang-format on
};

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  auto text = to_span(R"( { "zarg" : "whop", "foo" : [1,2,3] } )");
  TextContext ctx;
  auto tail = JsonParser::match(ctx, text);
  print_summary(text, tail, 40);

  return 0;
}

//------------------------------------------------------------------------------
