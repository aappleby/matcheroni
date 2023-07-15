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

using namespace matcheroni;

template<StringParam match_name, typename pattern, typename NodeType>
struct Capture3 {
  static cspan match(void* ctx, cspan s) {
    return Capture<match_name, pattern, NodeType>::match(ctx, s);
  }
};

//------------------------------------------------------------------------------
// To match anything at all, we first need to tell Matcheroni how to compare
// one atom of our input sequence against a constant.

static cspan match_regex(void* ctx, cspan s);

// Our 'control' characters consist of all atoms with special regex meanings.

struct cchar {
  using pattern = Atom<'\\', '(', ')', '|', '$', '.', '+', '*', '?', '[', ']', '^'>;
  static cspan match(void* ctx, cspan s) { return pattern::match(ctx, s); }
};

// Our 'plain' characters are every character that's not a control character.

struct pchar {
  using pattern = Seq<Not<cchar>, AnyAtom>;
  static cspan match(void* ctx, cspan s) { return pattern::match(ctx, s); }
};

// Plain text is any span of plain characters not followed by an operator.

struct text {
  using pattern = Some<Seq<pchar, Not<Atom<'*', '+', '?'>>>>;
  static cspan match(void* ctx, cspan s) { return pattern::match(ctx, s); }
};

// Our 'meta' characters are anything after a backslash.

struct mchar {
  using pattern = Seq<Atom<'\\'>, AnyAtom>;
  static cspan match(void* ctx, cspan s) { return pattern::match(ctx, s); }
};

// A character range is a beginning character and an end character separated
// by a hyphen.


struct range {
  using pattern = Seq<
    Capture3<"begin", pchar, TextNode>,
    Atom<'-'>,
    Capture3<"end", pchar, TextNode>
  >;
  static cspan match(void* ctx, cspan s) { return pattern::match(ctx, s); }
};

// The contents of a matcher set must be ranges or individual characters.
struct set_body {
  static cspan match(void* ctx, cspan s) {
    return
    Some<
      Capture3<"range", range, TextNode>,
      Capture3<"char",  pchar, TextNode>,
      Capture3<"meta",  mchar, TextNode>
    >::match(ctx, s);
  }
};


// The regex units that we can apply a */+/? operator to are sets, groups,
// dots, and single characters.
// Note that "group" recurses through RegexParser::match.

struct unit {
  using pattern =
  Oneof<
    Capture3<"neg_set", Seq<Atom<'['>, Atom<'^'>, set_body, Atom<']'>>, TextNode>,
    Capture3<"pos_set", Seq<Atom<'['>,            set_body, Atom<']'>>, TextNode>,
    Capture3<"group",   Seq<Atom<'('>, Ref<match_regex>,          Atom<')'>>, TextNode>,
    Capture3<"dot",     Atom<'.'>, TextNode>,
    Capture3<"char",    pchar, TextNode>,
    Capture3<"meta",    mchar, TextNode>
  >;
  static cspan match(void* ctx, cspan s) { return pattern::match(ctx, s); }
};

// A 'simple' regex is text, line end markers, a unit w/ operator, or a bare
// unit.

struct simple {
  using pattern = Some<
    Capture3<"text", text, TextNode>,
    Capture3<"BOL",  Atom<'^'>, TextNode>,
    Capture3<"EOL",  Atom<'$'>, TextNode>,
    Capture3<"any",  Seq<unit, Atom<'*'>>, TextNode>,
    Capture3<"some", Seq<unit, Atom<'+'>>, TextNode>,
    Capture3<"opt",  Seq<unit, Atom<'?'>>, TextNode>,
    unit
  >;
  static cspan match(void* ctx, cspan s) {
    return pattern::match(ctx, s);
  }
};

// A 'one-of' regex is a list of simple regexes separated by '|'.

struct oneof {
  using pattern =
  Seq<
    Capture3<"option", simple, TextNode>,
    Some<Seq<
      Atom<'|'>,
      Capture3<"option", simple, TextNode>
    >>
  >;
  static cspan match(void* ctx, cspan s) {
    return pattern::match(ctx, s);
  }
};

// This is the top level of our regex parser.
static cspan match_regex(void* ctx, cspan s) {
  // A 'top-level' regex is either a simple regex or a one-of regex.
  using regex_top =
  Oneof<
    Capture3<"oneof", oneof, TextNode>,
    simple
  >;
  return regex_top::match(ctx, s);
}

__attribute__((noinline))
cspan parse_regex(void* ctx, cspan s) {
  return match_regex(ctx, s);
  //return s.fail();
}

//------------------------------------------------------------------------------
