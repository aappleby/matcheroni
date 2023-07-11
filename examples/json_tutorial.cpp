//------------------------------------------------------------------------------
// This file is a full working example of using Matcheroni to build a JSON
// parser.

// Example usage:
// bin/json_tutorial test_file.json

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

using namespace matcheroni;

using cspan = Span<const char>;

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
  print_flat(node->span.a, node->span.b, 20);

  // Print out the name of the type name of the node with indentation.
  printf("   ");
  for (int i = 0; i < depth; i++) printf(i == depth-1 ? "|--" : "|  ");
  printf("%s\n", node->match_name);
  for (auto c = node->child_head; c; c = c->node_next) {
    print_tree(c, depth+1);
  }
}

//------------------------------------------------------------------------------
// To build a parse tree, we wrap the patterns we want to create nodes for
// in a Capture<> matcher that will invoke our node factory. We can also wrap
// them in a Trace<> matcher if we want to debug our patterns.

#ifdef TRACE

template<StringParam match_name, typename P>
using Capture = CaptureNamed<match_name, Trace<match_name, P>, NodeBase>;

#else

template<StringParam match_name, typename P>
using Capture = CaptureNamed<match_name, P, NodeBase>;

#endif

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

cspan match_value(void* ctx, cspan s);
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

cspan match_value(void* ctx, cspan s) {
  using value =
  Oneof<
    Capture<"array",   array>,
    Capture<"number",  number>,
    Capture<"object",  object>,
    Capture<"string",  string>,
    Capture<"keyword", keyword>
  >;
  return value::match(ctx, s);
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

  struct stat statbuf;
  int stat_result = stat(path, &statbuf);
  if (stat_result == -1) {
    printf("Could not open %s\n", path);
    return -1;
  }

  auto buf = new char[statbuf.st_size + 1];
  FILE* f = fopen(path, "rb");
  auto _ = fread(buf, statbuf.st_size, 1, f);
  buf[statbuf.st_size] = 0;
  fclose(f);


  printf("Parsing %s\n", path);

  Parser* parser = new Parser();


  cspan text = {buf, buf + statbuf.st_size};

  auto parse_end = json::match(parser, text);
  if (parse_end.valid()) {
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
