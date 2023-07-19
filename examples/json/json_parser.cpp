// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

//------------------------------------------------------------------------------

struct JsonParser {
  // clang-format off
  using ws        = Any<Atom<' ','\n','\r','\t'>>;
  using hex       = Range<'0','9','a','f','A','F'>;
  using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
  using keyword   = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;
  using character = Oneof< NotAtom<'"','\\'>, Seq<Atom<'\\'>, escape> >;
  using string    = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

  using sign      = Atom<'+','-'>;
  using digit     = Range<'0','9'>;
  using onenine   = Range<'1','9'>;
  using digits    = Some<digit>;
  using fraction  = Seq<Atom<'.'>, digits>;
  using exponent  = Seq<Atom<'e','E'>, Opt<sign>, digits>;
  using integer   = Seq< Opt<Atom<'-'>>, Oneof<Seq<onenine,digits>,digit> >;
  using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;

  template<typename P>
  using comma_separated = Seq<P, Any<Seq<ws, Atom<','>, ws, P>>>;

  static TextSpan value(TextNodeContext& ctx, TextSpan s) {
    return Oneof<
      Capture<"array",   array,   TextNode>,
      Capture<"number",  number,  TextNode>,
      Capture<"object",  object,  TextNode>,
      Capture<"string",  string,  TextNode>,
      Capture<"keyword", keyword, TextNode>
    >::match(ctx, s);
  }

  using pair =
  Seq<
    Capture<"key", string, TextNode>,
    ws,
    Atom<':'>,
    ws,
    Capture<"value", Ref<value>, TextNode>
  >;

  using object =
  Seq<
    Atom<'{'>, ws,
    Opt<comma_separated<
      Capture<"member", pair, TextNode>
    >>,
    ws,
    Atom<'}'>
  >;

  using array =
  Seq<
    Atom<'['>,
    ws,
    Opt<comma_separated<Ref<value>>>, ws,
    Atom<']'>
  >;

  // clang-format on
  //----------------------------------------

  static TextSpan match(TextNodeContext& ctx, TextSpan s) {
    return Seq<ws, Ref<value>, ws>::match(ctx, s);
  }
};


TextSpan parse_json(TextNodeContext& ctx, TextSpan s) {
  return JsonParser::match(ctx, s);
}

//------------------------------------------------------------------------------
