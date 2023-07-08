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

using namespace matcheroni;

// Uncomment this to print a full trace of the regex matching process. Note -
// the trace will be _very_ long, even for small regexes.
//#define TRACE

//------------------------------------------------------------------------------
// Our parse node for this example is pretty trivial - a type name, the
// endpoints of the matched text, and a list of child nodes.

struct Node {

  const char* type;
  const char* a;
  const char* b;
  std::vector<Node*> children;

  // Prints a text representation of the parse tree.
  // Example:
  // (^\d+\s+(very)?\s...   group
  // ^                      |--BOL
  // \d+                    |--some
  // \d                     |  |--meta

  void print_tree(int depth = 0) {

    // Print the node's matched text, with a "..." if it doesn't fit in 20
    // characters.

    int len = b-a;
    if (len > 20) {
      for (int i = 0;   i < 17; i++) putc(a[i], stdout);
      printf("...");
    }
    else {
      for (int i = 0;   i < len; i++) putc(a[i], stdout);
      for (int i = len; i < 20;  i++) putc(' ', stdout);
    }

    // Print out the name of the type name of the node with indentation.

    printf("   ");
    for (int i = 0; i < depth; i++) printf(i == depth-1 ? "|--" : "|  ");
    printf("%s\n", type);
    for (auto c : children) c->print_tree(depth+1);
  }
};

//------------------------------------------------------------------------------
// To convert our pattern matches to parse nodes, we create a Factory<> matcher
// that constructs a new NodeType() for a successful match, attaches any
// sub-nodes to it, and places it on a node stack.

// If this were a larger application, we would keep the node stack inside a
// match context object passed in via 'ctx', but a global is fine for now.

static std::vector<Node*> node_stack;

template<StringParam type, typename P, typename NodeType>
struct Factory {
  static const char* match(void* ctx, const char* a, const char* b) {
    if (!a || a == b) return nullptr;

    auto old_top = node_stack.size();
    auto end = P::match(ctx, a, b);
    auto new_top = node_stack.size();

    if (end) {
      auto new_node = new NodeType();
      new_node->type = type.str_val;
      new_node->a = a;
      new_node->b = end;
      for (int i = old_top; i < new_top; i++) {
        new_node->children.push_back(node_stack[i]);
      }
      node_stack.resize(old_top);
      node_stack.push_back(new_node);
    }

    return end;
  }
};

// There's one critical detail we need to make the factory work correctly - if
// we get partway through a match and then fail for some reason, we must
// "rewind" our match state back to the start of the failed match. This means
// we must also throw away any parse nodes that were created during the failed
// match.

// Matcheroni's default rewind callback does nothing, but if we provide a
// specialized version of it Matcheroni will call it as needed.

template<>
void matcheroni::atom_rewind(void* ctx, const char* a, const char* b) {
  while(node_stack.size() && node_stack.back()->b > a) {
    delete node_stack.back();
    node_stack.pop_back();
  }
}

//------------------------------------------------------------------------------
// To debug our patterns, we create a Trace<> matcher that prints out a diagram
// of the current match context, the matchers being tried, and whether they
// succeeded.

// Our trace depth is a global for convenience, same thing as node_stack above.

// Example snippet:

// {(good|bad)\s+[a-z]*$} |  pos_set ?
// {(good|bad)\s+[a-z]*$} |  pos_set X
// {(good|bad)\s+[a-z]*$} |  group ?
// {good|bad)\s+[a-z]*$ } |  |  oneof ?
// {good|bad)\s+[a-z]*$ } |  |  |  text ?
// {good|bad)\s+[a-z]*$ } |  |  |  text OK

static int trace_depth = 0;

template<StringParam type, typename P>
struct Trace {

  static void print_bar(const char* a, const char* suffix) {
    printf("{%-20.20s}   ", a);
    for (int i = 0; i < trace_depth; i++) printf("|  ");
    printf("%s %s\n", type.str_val, suffix);
  }

  static const char* match(void* ctx, const char* a, const char* b) {
    if (!a || a == b) return nullptr;
    print_bar(a, "?");
    trace_depth++;
    auto end = P::match(ctx, a, b);
    trace_depth--;
    print_bar(a, end ? "OK" : "X");
    return end;
  }
};

//------------------------------------------------------------------------------
// To build a parse tree, we wrap the patterns we want to create nodes for
// in a Capture<> matcher that will invoke our node factory. We can also wrap
// them in a Trace<> matcher if we want to debug our patterns.

#ifdef TRACE
template<StringParam type, typename P>
using Capture = Factory<type, Trace<type, P>, Node>;
#else
template<StringParam type, typename P>
using Capture = Factory<type, P, Node>;
#endif

//------------------------------------------------------------------------------
// To match anything at all, we first need to tell Matcheroni how to compare
// one atom of our input sequence against a constant.

// Matcheroni's built-in implementation of atom_cmp will work in this example,
// but we can also provide a specialized one.

template<>
inline int matcheroni::atom_cmp(void* ctx, const char* a, char b) {
  return int(*a - b);
}

// Regexes can be recursive, so to handle recursion we forward-declare a
// top-level matcher that we'll define later.

const char* match_regex(void* ctx, const char* a, const char* b);
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

const char* match_regex(void* ctx, const char* a, const char* b) {
  return regex_top::match(ctx, a, b);
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

  // Bash will un-quote the regex on the command line for us, so we don't need
  // to do any processing here.

  const char* regex = argv[1];

  // Invoke our regex matcher against the input text. If it matches, we will
  // get a non-null endpoint for the match.

  auto end = regex::match(nullptr, regex, regex + strlen(regex));
  printf("\n");

  // If everything went well, our node stack should now have a sequence of
  // parse nodes in it.

  if (!end) {
    printf("Our matcher could not match anything!\n");
    return 1;
  }
  else if (node_stack.empty()) {
    printf("No parse nodes created!\n");
    return 1;
  }
  else {
    printf("Parse tree:\n");
    for (auto n : node_stack) n->print_tree();

    if (*end != 0) {
      printf("Leftover text: {%s}\n", end);
    }
    return 0;
  }
}

//------------------------------------------------------------------------------
