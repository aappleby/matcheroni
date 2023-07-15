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

//------------------------------------------------------------------------------
// To match anything at all, we first need to tell Matcheroni how to compare
// one atom of our input sequence against a constant.

struct RegexParser {
  // clang-format off

  // This is the top level of our regex parser.
  static cspan match(void* ctx, cspan s) {
    // A 'top-level' regex is either a simple regex or a one-of regex.
    using regex_top =
    Oneof<
      Capture<"oneof", oneof, TextNode>,
      simple
    >;
    return regex_top::match(ctx, s);
  }

  // Our 'control' characters consist of all atoms with special regex meanings.

  using cchar = Atom<'\\', '(', ')', '|', '$', '.', '+', '*', '?', '[', ']', '^'>;

  // Our 'plain' characters are every character that's not a control character.

  using pchar = Seq<Not<cchar>, AnyAtom>;

  // Plain text is any span of plain characters not followed by an operator.

  using text = Some<Seq<pchar, Not<Atom<'*', '+', '?'>>>>;

  // Our 'meta' characters are anything after a backslash.

  using mchar = Seq<Atom<'\\'>, AnyAtom>;

  // A character range is a beginning character and an end character separated
  // by a hyphen.

  using range =
  Seq<
    Capture<"begin", pchar, TextNode>,
    Atom<'-'>,
    Capture<"end", pchar, TextNode>
  >;

  // The contents of a matcher set must be ranges or individual characters.

  using set_body =
  Some<
    Capture<"range", range, TextNode>,
    Capture<"char",  pchar, TextNode>,
    Capture<"meta",  mchar, TextNode>
  >;

  // The regex units that we can apply a */+/? operator to are sets, groups,
  // dots, and single characters.
  // Note that "group" recurses through RegexParser::match.

  using unit =
  Oneof<
    Capture<"neg_set", Seq<Atom<'['>, Atom<'^'>, set_body, Atom<']'>>, TextNode>,
    Capture<"pos_set", Seq<Atom<'['>,            set_body, Atom<']'>>, TextNode>,
    Capture<"group",   Seq<Atom<'('>, Ref<match>,          Atom<')'>>, TextNode>,
    Capture<"dot",     Atom<'.'>, TextNode>,
    Capture<"char",    pchar, TextNode>,
    Capture<"meta",    mchar, TextNode>
  >;

  // A 'simple' regex is text, line end markers, a unit w/ operator, or a bare
  // unit.

  using simple = Some<
    Capture<"text", text, TextNode>,
    Capture<"BOL",  Atom<'^'>, TextNode>,
    Capture<"EOL",  Atom<'$'>, TextNode>,
    Capture<"any",  Seq<unit, Atom<'*'>>, TextNode>,
    Capture<"some", Seq<unit, Atom<'+'>>, TextNode>,
    Capture<"opt",  Seq<unit, Atom<'?'>>, TextNode>,
    unit
  >;

  // A 'one-of' regex is a list of simple regexes separated by '|'.

  using oneof =
  Seq<
    Capture<"option", simple, TextNode>,
    Some<Seq<
      Atom<'|'>,
      Capture<"option", simple, TextNode>
    >>
  >;

  // clang-format on
};

cspan parse_regex(void* ctx, cspan s) {
  return RegexParser::match(ctx, s);
}

//------------------------------------------------------------------------------
