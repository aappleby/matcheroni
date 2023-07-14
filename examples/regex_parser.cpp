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
      Capture<"oneof", oneof, NodeBase>,
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
    Capture<"begin", pchar, NodeBase>,
    Atom<'-'>,
    Capture<"end", pchar, NodeBase>
  >;

  // The contents of a matcher set must be ranges or individual characters.

  using set_body =
  Some<
    Capture<"range", range, NodeBase>,
    Capture<"char",  pchar, NodeBase>,
    Capture<"meta",  mchar, NodeBase>
  >;

  // The regex units that we can apply a */+/? operator to are sets, groups,
  // dots, and single characters.
  // Note that "group" recurses through RegexParser::match.

  using unit =
  Oneof<
    Capture<"neg_set", Seq<Atom<'['>, Atom<'^'>, set_body, Atom<']'>>, NodeBase>,
    Capture<"pos_set", Seq<Atom<'['>,            set_body, Atom<']'>>, NodeBase>,
    Capture<"group",   Seq<Atom<'('>, Ref<match>,          Atom<')'>>, NodeBase>,
    Capture<"dot",     Atom<'.'>, NodeBase>,
    Capture<"char",    pchar, NodeBase>,
    Capture<"meta",    mchar, NodeBase>
  >;

  // A 'simple' regex is text, line end markers, a unit w/ operator, or a bare
  // unit.

  using simple = Some<
    Capture<"text", text, NodeBase>,
    Capture<"BOL",  Atom<'^'>, NodeBase>,
    Capture<"EOL",  Atom<'$'>, NodeBase>,
    Capture<"any",  Seq<unit, Atom<'*'>>, NodeBase>,
    Capture<"some", Seq<unit, Atom<'+'>>, NodeBase>,
    Capture<"opt",  Seq<unit, Atom<'?'>>, NodeBase>,
    unit
  >;

  // A 'one-of' regex is a list of simple regexes separated by '|'.

  using oneof =
  Seq<
    Capture<"option", simple, NodeBase>,
    Some<Seq<
      Atom<'|'>,
      Capture<"option", simple, NodeBase>
    >>
  >;

  // clang-format on
};

cspan parse_regex(void* ctx, cspan s) {
  return RegexParser::match(ctx, s);
}

//------------------------------------------------------------------------------
