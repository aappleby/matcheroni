//------------------------------------------------------------------------------
// This file is a full working example of using Matcheroni to build a JSON
// parser.

// Example usage:
// bin/json_parser test_file.json

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Parseroni.hpp"

#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>

#ifdef DEBUG
#define DCHECK(A) assert(A)
#else
#define DCHECK(A)
#endif

using namespace matcheroni;

constexpr bool verbose       = false;
constexpr bool dump_tree     = true;

double byte_accum = 0;
double time_accum = 0;
double line_accum = 0;
const int warmup = 10;
const int reps = 10;

//------------------------------------------------------------------------------

double timestamp_ms() {
  timespec t;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
  return  double(t.tv_sec) * 1e3 + double(t.tv_nsec) * 1e-6;
}

//------------------------------------------------------------------------------

struct JsonNode : public NodeBase {
  using NodeBase::NodeBase;
  const char* matcher_name;
};

//------------------------------------------------------------------------------
// Prints a text representation of the parse tree.

void print_tree(JsonNode* node, int depth = 0) {
  //if (depth > 3) return;

  // Print the node's matched text, with a "..." if it doesn't fit in 20
  // characters.
  //print_flat(node->a, node->b, 20);

  // Print out the name of the type name of the node with indentation.

  printf("   ");
  for (int i = 0; i < depth; i++) printf(i == depth-1 ? "|--" : "|  ");
  printf("%s\n", node->matcher_name);
  for (JsonNode* c = (JsonNode*)node->child_head; c; c = (JsonNode*)c->node_next) {
    print_tree(c, depth+1);
  }
}

//------------------------------------------------------------------------------
// To build a parse tree, we wrap the patterns we want to create nodes for
// in a Capture<> matcher that will invoke our node factory. We can also wrap
// them in a Trace<> matcher if we want to debug our patterns.

#ifdef TRACE

template<StringParam matcher_name, typename P>
using Capture = CaptureNode<matcher_name, Trace<matcher_name, P>, JsonNode>;

#else

template<StringParam matcher_name, typename P>
using Capture = CaptureNode<matcher_name, P, JsonNode>;

#endif

//------------------------------------------------------------------------------

using ws        = Any<Atom<' ','\n','\r','\t'>>;
using hex       = Oneof<Range<'0','9'>, Range<'A','F'>, Range<'a','f'>>;
using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
using keyword   = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;
using character = Oneof< NotAtom<'"','\\'>, Seq<Atom<'\\'>, escape> >;
using string    = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

using sign     = Atom<'+','-'>;
using digit    = Range<'0','9'>;
using onenine  = Range<'1','9'>;
using digits   = Some<digit>;
using fraction = Seq<Atom<'.'>, digits>;
using exponent = Seq<Atom<'e','E'>, Opt<sign>, digits>;
using integer  = Seq< Opt<Atom<'-'>>, Oneof<Seq<onenine,digits>,digit> >;
using number   = Seq<integer, Opt<fraction>, Opt<exponent>>;

const char* match_value(void* ctx, const char* a, const char* b);
using value = Ref<match_value>;

using member =
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
  Opt<comma_separated<Capture<"member", member>>>, ws,
  Atom<'}'>
>;

using array =
Seq<
  Atom<'['>,
  ws,
  Opt<comma_separated<value>>, ws,
  Atom<']'>
>;

const char* match_value(void* ctx, const char* a, const char* b) {
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

const char* match_json(void* ctx, const char* a, const char* b) {
  return Seq<ws, value, ws>::match(ctx, a, b);
}

//------------------------------------------------------------------------------

int test_parser(Parser* parser, const char* text_a, const char* text_b) {
  parser->reset();

  double time = -timestamp_ms();
  const char* end = match_json(parser, text_a, text_b);
  time += timestamp_ms();
  time_accum += time;

  int result = 0;

  if (verbose) {
    printf("Parse done in %f msec\n", time);
    printf("\n");
  }

  if (!end) {
    printf("Parser could not match anything\n");
    result = -1;
  }

  if (*end != 0) {
    printf("Leftover text: |%s|\n", end);
    result = -1;
  }

  if (parser->top_head == nullptr) {
    printf("No parse nodes created\n");
    result = -1;
  }

  return result;
}

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
    "../nativejson-benchmark/data/canada.json",
    "../nativejson-benchmark/data/citm_catalog.json",
    "../nativejson-benchmark/data/twitter.json",
  };

  Parser* parser = new Parser();

  for (int rep = 0; rep < (warmup + reps); rep++) {
    if (rep == warmup) {
      byte_accum = 0;
      time_accum = 0;
      line_accum = 0;
    }

    for (auto path : paths) {
      if (verbose) {
        printf("\n\n----------------------------------------\n");
        printf("Parsing %s\n", path);
      }

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

      //parser->text = buf;
      //parser->text_size = statbuf.st_size;
      byte_accum += statbuf.st_size;

      const char* text_a = buf;
      const char* text_b = buf + statbuf.st_size;

      for (auto c = text_a; c < text_b; c++) if (*c == '\n') line_accum++;

      test_parser(parser, text_a, text_b);

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
