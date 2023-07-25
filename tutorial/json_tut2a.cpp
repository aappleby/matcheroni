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

/*
json
  element

value
  object
  array
  string
  number
  "true"
  "false"
  "null"

object
  '{' ws '}'
  '{' members '}'

members
  member
  member ',' members

member
  ws string ws ':' element

array
  '[' ws ']'
  '[' elements ']'

elements
  element
  element ',' elements

element
  ws value ws

string
  '"' characters '"'

characters
  ""
  character characters

character
  '0020' . '10FFFF' - '"' - '\'
  '\' escape

escape
  '"'
  '\'
  '/'
  'b'
  'f'
  'n'
  'r'
  't'
  'u' hex hex hex hex

hex
  digit
  'A' . 'F'
  'a' . 'f'

ws
  ""
  '0020' ws
  '000A' ws
  '000D' ws
  '0009' ws
*/

//------------------------------------------------------------------------------
// To build a parse tree, we wrap the patterns we want to create nodes for
// in a Capture<> matcher that will invoke our node factory. We can also wrap
// them in a TraceText<> matcher if we want to debug our patterns.

struct JsonParser {



  using onenine   = Range<'1','9'>;
  using digit     = Oneof<Atom<'0'>, onenine>;
  using digits    = Some<digit>;
  using integer   = Seq< Opt<Atom<'-'>>, Oneof<Seq<onenine,digits>,digit> >;
  using fraction  = Opt<Seq<Atom<'.'>, digits>>;
  using sign      = Opt<Atom<'+','-'>>;
  using exponent  = Opt<Seq<Atom<'e','E'>, sign, digits>>;
  using number    = Seq<integer, fraction, exponent>;




  using space     = Some<Atom<' ','\n','\r','\t'>>;
  using hex       = Range<'0','9','a','f','A','F'>;
  using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
  using keyword   = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;
  using character = Seq<Not<Atom<'\"'>>, Not<Atom<'\\'>>, Range<0x0020, 0x10FFFF>>;
  using string    = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;


  template<typename P>
  using list = Opt<Seq<P, Any<Seq<Any<space>, Atom<','>, Any<space>, P>>>>;

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
    Any<space>,
    list<Ref<value>>,
    Any<space>,
    Atom<']'>
  >;

  using pair =
  Seq<
    Capture<"key", string, TextNode>,
    Any<space>,
    Atom<':'>,
    Any<space>,
    Capture<"value", Ref<value>, TextNode>
  >;

  using object =
  Seq<
    Atom<'{'>,
    Any<space>,
    list<Capture<"pair", pair, TextNode>>,
    Any<space>,
    Atom<'}'>
  >;

  static TextSpan match(TextNodeContext& ctx, TextSpan body) {
    return Seq<Any<space>, Ref<value>, Any<space>>::match(ctx, body);
  }
};

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("json_tut1a <filename>\n");
    return 0;
  }

  printf("argv[0] = %s\n", argv[0]);
  printf("argv[1] = %s\n", argv[1]);
  printf("\n");

  TextNodeContext ctx;
  auto input = read(argv[1]);
  auto text = to_span(input);
  auto tail = JsonParser::match(ctx, text);
  print_summary(text, tail, ctx, 50);

  return tail.is_valid() ? 0 : -1;
}

//------------------------------------------------------------------------------
