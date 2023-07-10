//------------------------------------------------------------------------------
// This file is a full working example of using Matcheroni to build a JSON
// parser.

// Example usage:
// bin/json_parser test_file.json

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

using namespace matcheroni;

using CResult = Span<const char>;

//#define TRACE
//#define TRACE
//#define FORCE_REWINDS

constexpr bool verbose   = false;
constexpr bool dump_tree = false;

const int warmup = 10;
const int reps = 10;

//------------------------------------------------------------------------------

double timestamp_ms() {
  timespec t;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
  return double(t.tv_sec) * 1e3 + double(t.tv_nsec) * 1e-6;
}

//------------------------------------------------------------------------------

struct JsonNode : public NodeBase {
  using NodeBase::NodeBase;
};

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

void print_tree(JsonNode* node, int depth = 0) {
  // Print the node's matched text, with a "..." if it doesn't fit in 20
  // characters.
  print_flat(node->match_a, node->match_b, 20);

  // Print out the name of the type name of the node with indentation.
  printf("   ");
  for (int i = 0; i < depth; i++) printf(i == depth-1 ? "|--" : "|  ");
  printf("%s\n", node->match_name);
  for (JsonNode* c = (JsonNode*)node->child_head; c; c = (JsonNode*)c->node_next) {
    print_tree(c, depth+1);
  }
}

//------------------------------------------------------------------------------
// To build a parse tree, we wrap the patterns we want to create nodes for
// in a Capture<> matcher that will invoke our node factory. We can also wrap
// them in a Trace<> matcher if we want to debug our patterns.

#ifdef TRACE

template<StringParam match_name, typename P>
using Capture = CaptureNamed<match_name, Trace<match_name, P>, JsonNode>;

#else

template<StringParam match_name, typename P>
using Capture = CaptureNamed<match_name, P, JsonNode>;

#endif

//------------------------------------------------------------------------------

using ws        = Any<Atom<' ','\n','\r','\t'>>;
using hex       = Oneof<Range<'0','9'>, Range<'A','F'>, Range<'a','f'>>;
using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
using keyword   = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;
using character = Oneof< NotAtom<'"','\\'>, Seq<Atom<'\\'>, escape> >;
using string    = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

using sign      = Atom<'+','-'>;
using digit     = Range<'0','9'>;
using onenine   = Range<'1','9'>;
using digits    = Some<digit>;
using fraction  = Seq<Atom<'.'>, digits>;
using exponent  = Seq<Atom<'e','E'>, Opt<sign>, digits>;
using integer   = Seq< Opt<Atom<'-'>>, Oneof<Seq<onenine,digits>,digit> >;
using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;

CResult match_value(void* ctx, const char* a, const char* b);
using value = Ref<match_value>;

using pair =
Seq<
  Capture<"key", string>,
  ws,
  Atom<':'>,
  ws,
  Capture<"value", value>
>;

template<typename P>
using comma_separated = Seq<P, Any<Seq<ws, Atom<','>, ws, P>>>;

using object =
Seq<
  Atom<'{'>, ws,
  Opt<comma_separated<Capture<"member", pair>>>, ws,
  Atom<'}'>
>;

using array =
Seq<
  Atom<'['>,
  ws,
  Opt<comma_separated<value>>, ws,
  Atom<']'>
>;

CResult match_value(void* ctx, const char* a, const char* b) {
  using value =
  Oneof<
    Capture<"array",   array>,
#ifdef FORCE_REWINDS
    Capture<"blee",    Seq<Capture<"blah", number>,Lit<"thisisbad">>>,
#endif
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

  /*
  if (argc < 2) {
    printf("Usage: json_parser <filename>\n");
    return 1;
  }

  printf("argv[0] = %s\n", argv[0]);
  printf("argv[1] = %s\n", argv[1]);
  //paths.push_back(argv[1]);
  */

  const char* paths[] = {
    //"data/test.json",

    // 4609770.000000
    "../nativejson-benchmark/data/canada.json",
    "../nativejson-benchmark/data/citm_catalog.json",
    "../nativejson-benchmark/data/twitter.json",
  };

  double byte_accum = 0;
  double time_accum = 0;
  double line_accum = 0;

  Parser* parser = new Parser();

  for (auto path : paths) {
    if (verbose) {
      printf("\n\n----------------------------------------\n");
      printf("Parsing %s\n", path);
    }

    struct stat statbuf;
    int stat_result = stat(path, &statbuf);
    if (stat_result == -1) {
      printf("Could not open %s\n", path);
      continue;
    }

    auto buf = new char[statbuf.st_size + 1];
    FILE* f = fopen(path, "rb");
    auto _ = fread(buf, statbuf.st_size, 1, f);
    buf[statbuf.st_size] = 0;
    fclose(f);

    byte_accum += statbuf.st_size;
    for (int i = 0; i < statbuf.st_size; i++) if (buf[i] == '\n') line_accum++;

    const char* text_a = buf;
    const char* text_b = buf + statbuf.st_size;
    CResult parse_end = {nullptr, nullptr};

    //----------------------------------------

    double path_time_accum = 0;

    for (int rep = 0; rep < (warmup + reps); rep++) {
      parser->reset();

      double time_a = timestamp_ms();
      parse_end = json::match(parser, text_a, text_b);
      double time_b = timestamp_ms();

      if (rep >= warmup) path_time_accum += time_b - time_a;
    }

    time_accum += path_time_accum;

    //----------------------------------------

    if (parse_end < text_b) {
      printf("Parse failed!\n");
      continue;
    }

    if (verbose) {
      printf("Parsed %d reps in %f msec\n", reps, path_time_accum);
      printf("\n");
    }

    if (dump_tree) {
      printf("Parse tree:\n");
      for (auto n = parser->top_head; n; n = n->node_next) {
        print_tree((JsonNode*)n);
      }
    }

    if (verbose) {
      printf("Slab current      %d\n",  NodeBase::slabs.current_size);
      printf("Slab max          %d\n",  NodeBase::slabs.max_size);
      printf("Tree nodes        %ld\n", parser->node_count());
      printf("Constructor calls %ld\n", NodeBase::constructor_calls);
      printf("Destructor calls  %ld\n", NodeBase::destructor_calls);
    }

    delete [] buf;
  }

  printf("\n");
  printf("----------------------------------------\n");
  printf("Byte accum %f\n", byte_accum);
  printf("Time accum %f\n", time_accum);
  printf("Line accum %f\n", line_accum);
  printf("Byte rate  %f\n", byte_accum / (time_accum / 1000.0));
  printf("Line rate  %f\n", line_accum / (time_accum / 1000.0));
  printf("Rep time   %f\n", time_accum / reps);
  printf("\n");

  delete parser;

  return 0;
}

//------------------------------------------------------------------------------
