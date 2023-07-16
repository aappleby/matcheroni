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
// them in a Trace<> matcher if we want to debug our patterns.

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
  using hex       = Oneof<Range<'0','9'>, Range<'A','F'>, Range<'a','f'>>;
  using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
  using keyword   = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;
  using character = Oneof< NotAtom<'"','\\'>, Seq<Atom<'\\'>, escape> >;
  using string    = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;
  // clang-format on

  template<typename P>
  using list = Opt<Seq<P, Any<Seq<ws, Atom<','>, ws, P>>>>;

  static cspan value(void* ctx, cspan s) {
    using value =
    Oneof<
      Capture<"array",   array,   TextNode>,
      Capture<"number",  number,  TextNode>,
      Capture<"object",  object,  TextNode>,
      Capture<"string",  string,  TextNode>,
      Capture<"keyword", keyword, TextNode>
    >;
    return value::match(ctx, s);
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

  static cspan match(void* ctx, cspan s) {
    return Seq<ws, Ref<value>, ws>::match(ctx, s);
  }
};

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: json_tutorial <filename>\n");
    return 1;
  }

  auto path = argv[1];

  printf("Loading %s\n", path);
  char* buf = nullptr;
  size_t size = 0;
  read(path, buf, size);

  printf("Parsing %s\n", path);
  TextContext* context = new TextContext();
  auto parse_end = JsonParser::match(context, cspan(buf, buf + size));

  if (parse_end.is_valid()) {
    printf("Parse tree:\n");
    for (auto n = context->top_head(); n; n = n->node_next()) {
      print_tree((TextNode*)n);
    }
  }
  else {
    printf("Parse failed!\n");
    return -1;
  }

  delete [] buf;
  delete context;
  return 0;
}

//------------------------------------------------------------------------------
