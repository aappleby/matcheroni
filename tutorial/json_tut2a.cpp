//------------------------------------------------------------------------------
// This file is a full working example of using Matcheroni to build a JSON
// parser.

// Example usage:
// bin/json_tutorial test_file.json

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

#include <stdio.h>

using namespace matcheroni;

//------------------------------------------------------------------------------
// To build a parse tree, we wrap the patterns we want to create nodes for
// in a Capture<> matcher that will invoke our node factory. We can also wrap
// them in a TraceText<> matcher if we want to debug our patterns.

struct JsonParser {
  // clang-format off
  using sign      = Atom<'+','-'>;
  using digit     = Range<'0','9'>;
  using onenine   = Range<'1','9'>;
  using digits    = Some<digit>;
  using integer   = Seq< Opt<Atom<'-'>>, Oneof<Seq<onenine,digits>,digit> >;
  using fraction  = Seq<Atom<'.'>, digits>;
  using exponent  = Seq<Atom<'e','E'>, Opt<sign>, digits>;
  using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;

  using ws        = Any<Atom<' ','\n','\r','\t'>>;
  using hex       = Range<'0','9','a','f','A','F'>;
  using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
  using keyword   = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;
  using character = Oneof< NotAtom<'"','\\'>, Seq<Atom<'\\'>, escape> >;
  using string    = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;
  // clang-format on

  template<typename P>
  using list = Opt<Seq<P, Any<Seq<ws, Atom<','>, ws, P>>>>;

  static TextSpan value(TextNodeContext& ctx, TextSpan body) {
    using value =
    Oneof<
      Capture<"array",   array,   TextNode>,
      Capture<"number",  number,  TextNode>,
      Capture<"object",  object,  TextNode>,
      Capture<"string",  string,  TextNode>,
      Capture<"keyword", keyword, TextNode>
    >;
    return value::match(ctx, body);
  }

  using array =
  Seq<
    Atom<'['>,
    ws,
    list<Ref<value>>,
    ws,
    Atom<']'>
  >;

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
    Atom<'{'>,
    ws,
    list<Capture<"pair", pair, TextNode>>,
    ws,
    Atom<'}'>
  >;

  static TextSpan match(TextNodeContext& ctx, TextSpan body) {
    return Seq<ws, Ref<value>, ws>::match(ctx, body);
  }
};

//------------------------------------------------------------------------------

const char* json = R"(
{
  "foo" : "bar",
  "baz" : [1, 2, 3, -5.238492834e-123],
  "blep" : true,
  "blap" : false,
  "blop" : null
}
)";

int main(int argc, char** argv) {
  TextNodeContext ctx;
  TextSpan text = to_span(json);
  auto tail = JsonParser::match(ctx, text);
  print_summary(text, tail, ctx, 40);

  return 0;
}

//------------------------------------------------------------------------------
