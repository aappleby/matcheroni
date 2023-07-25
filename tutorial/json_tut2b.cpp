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

struct NumberNode : public TextNode {
  void init(const char* match_name, TextSpan span, uint64_t flags) override {
    TextNode::init(match_name, span, flags);
    value = atof(span.begin);
  }

  using TextNode::TextNode;
  double value = 0;
};

//------------------------------------------------------------------------------
// To build a parse tree, we wrap the patterns we want to create nodes for
// in a Capture<> matcher that will invoke our node factory. We can also wrap
// them in a TraceText<> matcher if we want to debug our patterns.

struct JsonParser {
  using sign      = Atom<'+','-'>;
  using digit     = Range<'0','9'>;
  using onenine   = Range<'1','9'>;
  using digits    = Some<digit>;
  using integer   = Seq< Opt<Atom<'-'>>, Oneof<Seq<onenine,digits>,digit> >;
  using fraction  = Seq<Atom<'.'>, digits>;
  using exponent  = Seq<Atom<'e','E'>, Opt<sign>, digits>;
  using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;

  using space     = Some<Atom<' ','\n','\r','\t'>>;
  using hex       = Range<'0','9','a','f','A','F'>;
  using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
  using keyword   = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;
  using character = Oneof< NotAtom<'"','\\'>, Seq<Atom<'\\'>, escape> >;
  using string    = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

  template<typename P>
  using list = Opt<Seq<P, Any<Seq<Any<space>, Atom<','>, Any<space>, P>>>>;

  static TextSpan value(TextNodeContext& ctx, TextSpan body) {
    using value =
    Oneof<
      Capture<"array",   array,   TextNode>,
      Capture<"number",  number,  NumberNode>,
      Capture<"object",  object,  TextNode>,
      Capture<"string",  string,  TextNode>,
      Capture<"keyword", keyword, TextNode>
    >;
    return value::match(ctx, body);
  }

  using array  =
  Seq<
    Atom<'['>,
    Any<space>,
    list<Ref<value>>,
    Any<space>,
    Atom<']'>
  >;

  using pair   =
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

double sum(TextNode* node) {
  double result = 0;
  if (auto num = dynamic_cast<NumberNode*>(node)) {
    result += num->value;
  }
  for (auto n = node->child_head(); n; n = n->node_next()) {
    result += sum(n);
  }
  return result;
}

double sum(TextNodeContext& context) {
  double result = 0;
  for (auto n = context.top_head(); n; n = n->node_next()) {
    result += sum(n);
  }
  return result;
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("json_tut2b <filename>\n");
    return 0;
  }

  printf("argv[0] = %s\n", argv[0]);
  printf("argv[1] = %s\n", argv[1]);
  printf("\n");

  TextNodeContext ctx;
  auto input = utils::read(argv[1]);
  auto text = utils::to_span(input);
  auto tail = JsonParser::match(ctx, text);

  printf("Sum of number nodes: %f\n", sum(ctx));
  printf("\n");

  utils::print_summary(text, tail, ctx, 50);

  return tail.is_valid() ? 0 : -1;
}

//------------------------------------------------------------------------------
