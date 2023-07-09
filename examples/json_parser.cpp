//------------------------------------------------------------------------------
// This file is a full working example of using Matcheroni to build a JSON
// parser.


// Example usage:
// bin/json_parser test_file.json

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include <assert.h>
#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <memory.h>
#include "matcheroni/Parseroni.hpp"
#include <sys/stat.h>
#include <chrono>
#include <time.h>

#ifdef DEBUG
#define DCHECK(A) assert(A)
#else
#define DCHECK(A)
#endif

using namespace matcheroni;

//------------------------------------------------------------------------------

void read(const char* path, const char*& text_out, int& size_out) {
  struct stat statbuf;
  if (stat(path, &statbuf) != -1) {
    auto buf = new char[statbuf.st_size + 1];
    FILE* f = fopen(path, "rb");
    auto _ = fread(buf, statbuf.st_size, 1, f);
    buf[statbuf.st_size] = 0;
    fclose(f);

    text_out = buf;
    size_out = statbuf.st_size;
  }
}

//------------------------------------------------------------------------------

double timestamp_ms() {
  timespec t;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
  return  (double(t.tv_sec) * 1e9 + double(t.tv_nsec)) / 1e6;
}

//------------------------------------------------------------------------------

struct JsonNode : public NodeBase {

  using NodeBase::NodeBase;

  //----------------------------------------

  const char* field_name;
};


//----------------------------------------
// Prints a text representation of the parse tree.

void print_tree(JsonNode* node, int depth = 0) {
  //if (depth > 3) return;

  // Print the node's matched text, with a "..." if it doesn't fit in 20
  // characters.
  //print_flat(node->a, node->b, 20);

  // Print out the name of the type name of the node with indentation.

  printf("   ");
  for (int i = 0; i < depth; i++) printf(i == depth-1 ? "|--" : "|  ");
  printf("%s\n", node->field_name);
  for (JsonNode* c = (JsonNode*)node->child_head; c; c = (JsonNode*)c->node_next) {
    print_tree(c, depth+1);
  }
}



//------------------------------------------------------------------------------
// To build a parse tree, we wrap the patterns we want to create nodes for
// in a Capture<> matcher that will invoke our node factory. We can also wrap
// them in a Trace<> matcher if we want to debug our patterns.

#ifdef TRACE

template<StringParam type, typename P>
using Capture = JsonNodeFactory<type, Trace<type, P>>;

#else

template<StringParam field_name, typename P>
using Capture = CaptureNode<field_name, P, JsonNode>;


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

using test_rewind =
Seq<
  Capture<"blah", number>,
  Lit<"thisisbad">
>;

const char* match_value(void* ctx, const char* a, const char* b) {
  using value =
  Oneof<
    Capture<"array",   array>,
#ifdef FORCE_REWINDS
    Capture<"blee",    test_rewind>,
#endif
    CaptureNode<"number",  number, JsonNode>,
    Capture<"object",  object>,
    Capture<"string",  string>,
    Capture<"keyword", keyword>
  >;
  return value::match(ctx, a, b);
}

using json = Seq<ws, value, ws>;

__attribute__((noinline))
const char* match_json(void* ctx, const char* a, const char* b) {
  return json::match(ctx, a, b);
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

  double byte_accum = 0;
  double time_accum = 0;

  const int warmup = 10;
  const int reps = 10;

  Parser* parser = new Parser();

  for (int rep = 0; rep < (warmup + reps); rep++) {
    if (rep == warmup) {
      byte_accum = 0;
      time_accum = 0;
    }

    for (auto path : paths) {

      if (verbose) {
        printf("\n");
        printf("\n");
        printf("//----------------------------------------\n");
        printf("Parsing %s\n", path);
      }

      read(path, parser->text, parser->text_size);
      byte_accum += parser->text_size;
      //printf("Read %d bytes\n", text_size);

      const char* text_a = parser->text;
      const char* text_b = text_a + parser->text_size;

      parser->top_head = parser->top_tail = nullptr;

      NodeBase::slabs.reset();

      double time = -timestamp_ms();
      JsonNode::constructor_calls = 0;
      JsonNode::destructor_calls = 0;

      const char* end = match_json(parser, text_a, text_b);
      time += timestamp_ms();
      time_accum += time;

      if (verbose) {
        printf("Parse done in %f msec\n", time);
        printf("\n");
      }

      // If everything went well, our node stack should now have a sequence of
      // parse nodes in it.

      if (!end) {
        printf("Our matcher could not match anything!\n");
      }
      else if (parser->top_head == nullptr) {
        printf("No parse nodes created!\n");
      }
      else {
        if (dump_tree) {
          printf("Parse tree:\n");
          for (JsonNode* n = (JsonNode*)parser->top_head; n; n = (JsonNode*)n->node_next) {
            print_tree(n);
          }
        }

        if (*end != 0) {
          printf("Leftover text: |%s|\n", end);
        }
      }

      delete [] parser->text;


      if (verbose) {
        printf("Slab current      %d\n", NodeBase::slabs.current_size);
        printf("Slab max          %d\n", NodeBase::slabs.max_size);
        printf("Tree nodes        %ld\n", ((JsonNode*)parser->top_head)->node_count());
        if (count_nodes) {
          printf("Constructor calls %ld\n", JsonNode::constructor_calls);
          printf("Destructor calls  %ld\n", JsonNode::destructor_calls);
          printf("Live nodes        %ld\n", JsonNode::constructor_calls - JsonNode::destructor_calls);
        }
      }

      if (recycle_nodes) {
        if (verbose) {
          printf("\n");
          printf("----------recycle start----------\n");
        }

        parser->node_recycle(parser->top_head);
        DCHECK(constructor_calls == destructor_calls);

        if (verbose) {
          printf("----------recycle done----------\n");
          printf("\n");
          printf("Slab current      %d\n", NodeBase::slabs.current_size);
          printf("Slab max          %d\n", NodeBase::slabs.max_size);
          printf("Tree nodes        %ld\n", ((JsonNode*)parser->top_head)->node_count());
          if (count_nodes) {
            printf("Constructor calls %ld\n", JsonNode::constructor_calls);
            printf("Destructor calls  %ld\n", JsonNode::destructor_calls);
            printf("Live nodes        %ld\n", JsonNode::constructor_calls - JsonNode::destructor_calls);
          }
        }

        DCHECK(slabs.current_size == 0);
      }
    }
  }

  printf("\n");
  printf("----------------------------------------\n");
  printf("Byte accum %f\n", byte_accum);
  printf("Time accum %f\n", time_accum);
  printf("Byte rate  %f\n", byte_accum / (time_accum / 1000.0));
  printf("Rep time   %f\n", time_accum / reps);
  printf("\n");

  NodeBase::slabs.destroy();
  delete parser;

  return 0;
}

//------------------------------------------------------------------------------
