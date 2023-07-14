//------------------------------------------------------------------------------
// This file is a full working example of using Matcheroni to build a JSON
// parser.

// Example usage:
// bin/json_tutorial test_file.json

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

```
#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

using namespace matcheroni;
```

//------------------------------------------------------------------------------
// Prints a text representation of the parse tree.

void print_flat(const char* a, const char* b, int max_len) {
  int len = b - a;
  int span_len = max_len;
  if (len > max_len) span_len -= 3;

  for (int i = 0; i < span_len; i++) {
    if      (a + i >= b)   putc(' ',  stdout);
    else if (a[i] == '\n') putc(' ',  stdout);
    else if (a[i] == '\r') putc(' ',  stdout);
    else if (a[i] == '\t') putc(' ',  stdout);
    else                   putc(a[i], stdout);
  }

  if (len > max_len) printf("...");
}

void print_tree(NodeBase* node, int depth = 0) {
  // Print the node's matched text, with a "..." if it doesn't fit in 20
  // characters.
  print_flat(node->match_a, node->match_b, 20);

  // Print out the name of the type name of the node with indentation.
  printf("   ");
  for (int i = 0; i < depth; i++) printf(i == depth-1 ? "|--" : "|  ");
  printf("%s\n", node->match_name);
  for (auto c = node->child_head; c; c = c->node_next) {
    print_tree(c, depth+1);
  }
}

//------------------------------------------------------------------------------

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

template<typename P>
using comma_separated = Seq<P, Any<Seq<ws, Atom<','>, ws, P>>>;

const char* match_value(void* ctx, const char* a, const char* b);
using value = Ref<match_value>;

using array =
Seq<
  Atom<'['>,
  ws,
  Opt<comma_separated<value>>,
  ws,
  Atom<']'>
>;

using pair =
Seq<
  Capture<"key", string>,
  ws,
  Atom<':'>,
  ws,
  Capture<"value", value>
>;

using object =
Seq<
  Atom<'{'>,
  ws,
  Opt<comma_separated<Capture<"pair", pair>>>,
  ws,
  Atom<'}'>
>;

const char* match_value(void* ctx, const char* a, const char* b) {
  using value =
  Oneof<
    Capture<"array",   array>,
    Capture<"number",  number>,
    Capture<"object",  object>,
    Capture<"string",  string>,
    Capture<"keyword", keyword>
  >;
  return value::match(ctx, a, b);
}

using json = Seq<ws, value, ws>;

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: json_tutorial <filename>\n");
    return 1;
  }

  printf("argv[0] = %s\n", argv[0]);
  printf("argv[1] = %s\n", argv[1]);

  auto path = argv[1];

  printf("Loading %s\n", path);

  printf("Parsing %s\n", path);

  Context* context = new Parser();

  const char* text_a = buf;
  const char* text_b = buf + statbuf.st_size;

  if (auto parse_end = json::match(parser, text_a, text_b)) {
    printf("Parse tree:\n");
    for (auto n = parser->top_head; n; n = n->node_next) {
      print_tree(n);
    }
  }
  else {
    printf("Parse failed!\n");
    return -1;
  }

  delete [] buf;
  delete parser;
  return 0;
}

//------------------------------------------------------------------------------
