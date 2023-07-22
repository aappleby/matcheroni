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
  using space     = Some<Atom<' ', '\n', '\r', '\t'>>;
  using sign      = Atom<'+', '-'>;
  using digit     = Range<'0', '9'>;
  using onenine   = Range<'1', '9'>;
  using digits    = Some<digit>;
  using integer   = Seq<Opt<Atom<'-'>>, Oneof<Seq<onenine, digits>, digit>>;
  using fraction  = Seq<Atom<'.'>, digits>;
  using exponent  = Seq<Atom<'e', 'E'>, Opt<sign>, digits>;
  using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;
  using hex       = Range<'0','9','a','f','A','F'>;
  using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
  using character = Oneof<NotAtom<'"', '\\'>, Seq<Atom<'\\'>, escape>>;
  using string    = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;
  using keyword   = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;

  template <typename P>
  using list = Opt<Seq<P, Any<Seq<Any<space>, Atom<','>, Any<space>, P>>>>;

  static TextSpan value(TextContext& ctx, TextSpan body) {
    return
    Oneof<
      TraceText<"array",   array>,
      TraceText<"number",  number>,
      TraceText<"object",  object>,
      TraceText<"string",  string>,
      TraceText<"keyword", keyword>
    >::match(ctx, body);
  }

  using array =
  Seq<
    Atom<'['>,
    Any<space>,
    list<TraceText<"element", Ref<value>>>,
    Any<space>,
    Atom<']'>
  >;

  using pair =
  Seq<
    TraceText<"key", string>,
    Any<space>,
    Atom<':'>,
    Any<space>,
    TraceText<"val", Ref<value>>
  >;

  using object =
  Seq<
    Atom<'{'>,
    Any<space>,
    list<TraceText<"pair", pair>>,
    Any<space>,
    Atom<'}'>
  >;

  static TextSpan match(TextContext& ctx, TextSpan body) {
    return Seq<Any<space>, Ref<value>, Any<space>>::match(ctx, body);
  }
  // clang-format on
};

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("json_tut1c <filename>\n");
    return 0;
  }

  printf("argv[0] = %s\n", argv[0]);
  printf("argv[1] = %s\n", argv[1]);
  printf("\n");

  TextContext ctx;
  auto input = read(argv[1]);
  auto text = to_span(input);
  auto tail = JsonParser::match(ctx, text);
  print_summary(text, tail, 40);

  return 0;
}

//------------------------------------------------------------------------------
