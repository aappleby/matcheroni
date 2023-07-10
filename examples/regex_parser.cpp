//------------------------------------------------------------------------------
// This file is a full working example of using Matcheroni to build a parser
// that can parse a subset of regular expressions. Supported operators are
// ^, $, ., *, ?, +, |, (), [], [^], and escaped characters.

// Example usage:
// bin/regex_parser "(^\d+\s+(very)?\s+(good|bad)\s+[a-z]*$)"

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include <stdio.h>
#include <string.h>
#include <vector>
#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"

using namespace matcheroni;

typedef Span<const char> CResult;

//------------------------------------------------------------------------------
// To build a parse tree, we wrap the patterns we want to create nodes for
// in a Capture<> matcher that will invoke our node factory. We can also wrap
// them in a Trace<> matcher if we want to debug our patterns.

#ifdef TRACE
template<StringParam match_name, typename P>
using Capture = CaptureNamed<match_name, Trace<match_name, P>, Node>;
#else
template<StringParam match_name, typename P>
using Capture = CaptureNamed<match_name, P, NodeBase>;
#endif

//------------------------------------------------------------------------------
// To match anything at all, we first need to tell Matcheroni how to compare
// one atom of our input sequence against a constant.

// Matcheroni's built-in implementation of atom_cmp will work in this example,
// but we can also provide a specialized one.

// Regexes can be recursive, so to handle recursion we forward-declare a
// top-level matcher that we'll define later.

CResult match_regex(void* ctx, const char* a, const char* b);
using regex = Ref<match_regex>;

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
  Capture<"begin", pchar>,
  Atom<'-'>,
  Capture<"end", pchar>
>;

// The contents of a matcher set must be ranges or individual characters.

using set_body =
Some<
  Capture<"range", range>,
  Capture<"char",  pchar>,
  Capture<"meta",  mchar>
>;

// The regex units that we can apply a */+/? operator to are sets, groups, dots,
// and single characters.

using unit =
Oneof<
  Capture<"neg_set", Seq<Atom<'['>, Atom<'^'>, set_body, Atom<']'>>>,
  Capture<"pos_set", Seq<Atom<'['>,            set_body, Atom<']'>>>,
  Capture<"group",   Seq<Atom<'('>, regex, Atom<')'>>>,
  Capture<"dot",     Atom<'.'>>,
  Capture<"char",    pchar>,
  Capture<"meta",    mchar>
>;

// A 'simple' regex is text, line end markers, a unit w/ operator, or a bare
// unit.

using simple =
Some<
  Capture<"text", text>,
  Capture<"BOL",  Atom<'^'>>,
  Capture<"EOL",  Atom<'$'>>,
  Capture<"any",  Seq<unit, Atom<'*'>>>,
  Capture<"some", Seq<unit, Atom<'+'>>>,
  Capture<"opt",  Seq<unit, Atom<'?'>>>,
  unit
>;

// A 'one-of' regex is a list of simple regexes separated by '|'.

using oneof =
Seq<
  Capture<"option", simple>,
  Some<Seq<
    Atom<'|'>,
    Capture<"option", simple>
  >>
>;

// A 'top-level' regex is either a simple regex or a one-of regex.

using regex_top =
Oneof<
  Capture<"oneof", oneof>,
  simple
>;

// And lastly we implement the full-regex matcher we declared earlier.

CResult match_regex(void* ctx, const char* a, const char* b) {
  return regex_top::match(ctx, a, b);
}

//------------------------------------------------------------------------------
// Prints a text representation of the parse tree.

void print_tree(NodeBase* node, int depth = 0) {
  auto a = node->match_a;
  auto b = node->match_b;

  putc('`', stdout);
  for (int i = 0; i < 20; i++) {
    if      (i >= b - a)   putc(' ',  stdout);
    else if (a[i] == '\n') putc(' ',  stdout);
    else if (a[i] == '\r') putc(' ',  stdout);
    else if (a[i] == '\t') putc(' ',  stdout);
    else                   putc(a[i], stdout);
  }
  putc('`', stdout);

  printf(depth == 0 ? "  *" : "   ");
  for (int i = 0; i < depth; i++) printf(i == depth-1 ? "|--" : "|  ");
  printf("%s\n", node->match_name);
  for (auto c = node->child_head; c; c = c->node_next) print_tree(c, depth+1);
}

//------------------------------------------------------------------------------
// The demo app accepts a quoted regex as its first command line argument,
// attempts to parse it, and then prints out the resulting parse tree.

int main(int argc, char** argv) {

  if (argc < 2) {
    printf("Usage: regex_parser \"<your regex>\"\n");
    return 1;
  }

  printf("argv[0] = %s\n", argv[0]);
  printf("argv[1] = %s\n", argv[1]);
  printf("\n");

  // Bash will un-quote the regex on the command line for us, so we don't need
  // to do any processing here.
  auto regex = argv[1];

  printf("Parsing regex `%s`\n", regex);
  Parser* parser = new Parser();

  // Invoke our regex matcher against the input text. If it matches, we will
  // get a non-null endpoint for the match.
  auto parse_end = regex::match(parser, regex, regex + strlen(regex));

  printf("Parse end: %p\n", parse_end.a);
  printf("Parse leftovers: `%s`\n", parse_end.a);
  printf("\n");

  printf("Parse tree:\n");
  printf("----------------------------------------\n");
  for (auto n = parser->top_head; n; n = n->node_next) {
    print_tree(n);
  }
  printf("----------------------------------------\n");
  printf("\n");

  return 0;
}

//------------------------------------------------------------------------------
