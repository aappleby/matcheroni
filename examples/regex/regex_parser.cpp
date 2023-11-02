//------------------------------------------------------------------------------
// This file is a full working example of using Matcheroni to build a parser
// that can parse a subset of regular expressions. Supported operators are
// ^, $, ., *, ?, +, |, (), [], [^], and escaped characters.

// Example usage:
// bin/regex_parser "(^\d+\s+(very)?\s+(good|bad)\s+[a-z]*$)"

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;
using namespace parseroni;

template<StringParam match_name, typename pattern, typename node_type>
struct Capture3 {
  static TextSpan match(TextParseContext& ctx, TextSpan body) {
    return Capture<match_name, pattern, node_type>::match(ctx, body);
  }
};

//------------------------------------------------------------------------------
// To match anything at all, we first need to tell Matcheroni how to compare
// one atom of our input sequence against a constant.

static TextSpan match_regex(TextParseContext& ctx, TextSpan body);

// Our 'control' characters consist of all atoms with special regex meanings.

struct cchar {
  using pattern = Atoms<'\\', '(', ')', '|', '$', '.', '+', '*', '?', '[', ']', '^'>;
  static TextSpan match(TextParseContext& ctx, TextSpan body) { return pattern::match(ctx, body); }
};

// Our 'plain' characters are every character that's not a control character.

struct pchar {
  using pattern = Seq<Not<cchar>, AnyAtom>;
  static TextSpan match(TextParseContext& ctx, TextSpan body) { return pattern::match(ctx, body); }
};

// Plain text is any span of plain characters not followed by an operator.

struct text {
  using pattern = Some<Seq<pchar, Not<Atoms<'*', '+', '?'>>>>;
  static TextSpan match(TextParseContext& ctx, TextSpan body) { return pattern::match(ctx, body); }
};

// Our 'meta' characters are anything after a backslash.

struct mchar {
  using pattern = Seq<Atom<'\\'>, AnyAtom>;
  static TextSpan match(TextParseContext& ctx, TextSpan body) { return pattern::match(ctx, body); }
};

// A character range is a beginning character and an end character separated
// by a hyphen.


struct range {
  using pattern = Seq<
    Capture3<"begin", pchar, TextParseNode>,
    Atom<'-'>,
    Capture3<"end", pchar, TextParseNode>
  >;
  static TextSpan match(TextParseContext& ctx, TextSpan body) { return pattern::match(ctx, body); }
};

// The contents of a matcher set must be ranges or individual characters.
struct set_body {
  static TextSpan match(TextParseContext& ctx, TextSpan body) {
    return
    Some<
      Capture3<"range", range, TextParseNode>,
      Capture3<"char",  pchar, TextParseNode>,
      Capture3<"meta",  mchar, TextParseNode>
    >::match(ctx, body);
  }
};


// The regex units that we can apply a */+/? operator to are sets, groups,
// dots, and single characters.
// Note that "group" recurses through RegexParser::match.

struct unit {
  using pattern =
  Oneof<
    Capture3<"neg_set", Seq<Atom<'['>, Atom<'^'>, set_body, Atom<']'>>, TextParseNode>,
    Capture3<"pos_set", Seq<Atom<'['>,            set_body, Atom<']'>>, TextParseNode>,
    Capture3<"group",   Seq<Atom<'('>, Ref<match_regex>,          Atom<')'>>, TextParseNode>,
    Capture3<"dot",     Atom<'.'>, TextParseNode>,
    Capture3<"char",    pchar, TextParseNode>,
    Capture3<"meta",    mchar, TextParseNode>
  >;
  static TextSpan match(TextParseContext& ctx, TextSpan body) { return pattern::match(ctx, body); }
};

// A 'simple' regex is text, line end markers, a unit w/ operator, or a bare
// unit.

struct simple {
  using pattern = Some<
    Capture3<"text", text, TextParseNode>,
    Capture3<"BOL",  Atom<'^'>, TextParseNode>,
    Capture3<"EOL",  Atom<'$'>, TextParseNode>,
    Capture3<"any",  Seq<unit, Atom<'*'>>, TextParseNode>,
    Capture3<"some", Seq<unit, Atom<'+'>>, TextParseNode>,
    Capture3<"opt",  Seq<unit, Atom<'?'>>, TextParseNode>,
    unit
  >;
  static TextSpan match(TextParseContext& ctx, TextSpan body) {
    return pattern::match(ctx, body);
  }
};

// A 'one-of' regex is a list of simple regexes separated by '|'.

struct oneof {
  using pattern =
  Seq<
    Capture3<"option", simple, TextParseNode>,
    Some<Seq<
      Atom<'|'>,
      Capture3<"option", simple, TextParseNode>
    >>
  >;
  static TextSpan match(TextParseContext& ctx, TextSpan body) {
    return pattern::match(ctx, body);
  }
};

// This is the top level of our regex parser.
static TextSpan match_regex(TextParseContext& ctx, TextSpan body) {
  // A 'top-level' regex is either a simple regex or a one-of regex.
  using regex_top =
  Oneof<
    Capture3<"oneof", oneof, TextParseNode>,
    simple
  >;
  return regex_top::match(ctx, body);
}

TextSpan parse_regex(TextParseContext& ctx, TextSpan body) {
  return match_regex(ctx, body);
  //return body.fail();
}

//------------------------------------------------------------------------------
