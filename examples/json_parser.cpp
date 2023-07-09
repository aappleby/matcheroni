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

#ifdef DEBUG
#define DCHECK(A) assert(A)
#else
#define DCHECK(A)
#endif

using namespace matcheroni;

//------------------------------------------------------------------------------

void read(const char* path, char*& text_out, int& size_out) {
  struct stat statbuf;
  if (stat(path, &statbuf) != -1) {
    text_out = new char[statbuf.st_size + 1];
    size_out = statbuf.st_size;
    FILE* f = fopen(path, "rb");
    auto _ = fread(text_out, size_out, 1, f);
    text_out[statbuf.st_size] = 0;
    fclose(f);
  }
}

//------------------------------------------------------------------------------

double timestamp_ms() {
  using clock = std::chrono::high_resolution_clock;
  using nano = std::chrono::nanoseconds;

  static bool init = false;
  static double origin = 0;

  auto now = clock::now().time_since_epoch();
  auto now_nanos = std::chrono::duration_cast<nano>(now).count();
  if (!origin) origin = now_nanos;

  return (now_nanos - origin) * 1.0e-6;
}


//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------

struct JsonNode : public NodeBase {

  //----------------------------------------
  // Prints a text representation of the parse tree.

  void print_tree(int depth = 0) {
    //if (depth > 3) return;

    // Print the node's matched text, with a "..." if it doesn't fit in 20
    // characters.
    print_flat(a, b, 20);

    // Print out the name of the type name of the node with indentation.

    printf("   ");
    for (int i = 0; i < depth; i++) printf(i == depth-1 ? "|--" : "|  ");
    printf("%s\n", field_name);
    for (JsonNode* c = (JsonNode*)child_head; c; c = (JsonNode*)c->next) c->print_tree(depth+1);
  }

  //----------------------------------------

  const char* field_name;
};

//------------------------------------------------------------------------------

struct Parser {

  Parser() {}
  virtual ~Parser() {}

  //----------------------------------------

  void link_node(NodeBase* new_node, const char* a, const char* b, NodeBase* old_top_tail) {
    new_node->a = a;
    new_node->b = b;

    auto child_head = old_top_tail ? old_top_tail->next : top_head;
    auto child_tail = top_tail;

    new_node->child_head = child_head;
    new_node->child_tail = child_tail;

    if (!old_top_tail && top_tail) {
      // We are the new top node, enclose all the top nodes.
      new_node->prev = nullptr;
      new_node->next = nullptr;

      top_head = new_node;
    }
    else if (top_tail != old_top_tail) {
      // Enclose the nodes after old_top_tail.

      child_head->prev = nullptr;
      child_tail->next = nullptr;

      new_node->prev = old_top_tail;
      new_node->next = nullptr;

      old_top_tail->next = new_node;
    }
    else {
      // Nothing to enclose.
      new_node->child_head = nullptr;
      new_node->child_tail = nullptr;
      new_node->prev = nullptr;
      new_node->next = nullptr;

      if (top_tail) {
        new_node->prev = top_tail;
        top_tail->next = new_node;
      }
    }

    if (!top_head) top_head = new_node;
    top_tail = new_node;
  }

  //----------------------------------------
  // Nodes _must_ be deleted in the reverse order they were allocated.
  // In practice, this means we must delete the "parent" node first and then
  // must delete the child nodes from tail to head.

  void node_recycle(NodeBase* n) {
    auto old_tail = n->child_tail;
    delete n;

    auto c = old_tail;
    while(c) {
      auto prev = c->prev;
      node_recycle(c);
      c = prev;
    }
  }

  //----------------------------------------
  // To convert our pattern matches to parse nodes, we create a Factory<>
  // matcher that constructs a new NodeType() for a successful match, attaches
  // any sub-nodes to it, and places it on a node list.

  template<typename NodeType, typename P>
  NodeType* node_factory(const char* a, const char* b) {
    auto old_top_tail = top_tail;
    auto end = P::match(this, a, b);
    if (!end) return nullptr;

    auto new_node = new NodeType();
    link_node(new_node, a, end, old_top_tail);
    return new_node;
  }

  //----------------------------------------
  // There's one critical detail we need to make the factory work correctly - if
  // we get partway through a match and then fail for some reason, we must
  // "rewind" our match state back to the start of the failed match. This means
  // we must also throw away any parse nodes that were created during the failed
  // match.

  void rewind(const char* a) {
    while(top_tail && top_tail->b > a) {
      auto dead = top_tail;
      top_tail = (JsonNode*)top_tail->prev;
      if (recycle_nodes) {
        node_recycle(dead);
      }
    }
  }

  //----------------------------------------
  // To debug our patterns, we create a Trace<> matcher that prints out a
  // diagram of the current match context, the matchers being tried, and
  // whether they succeeded.

  // Example snippet:

  // {(good|bad)\s+[a-z]*$} |  pos_set ?
  // {(good|bad)\s+[a-z]*$} |  pos_set X
  // {(good|bad)\s+[a-z]*$} |  group ?
  // {good|bad)\s+[a-z]*$ } |  |  oneof ?
  // {good|bad)\s+[a-z]*$ } |  |  |  text ?
  // {good|bad)\s+[a-z]*$ } |  |  |  text OK

  // Uncomment this to print a full trace of the regex matching process. Note -
  // the trace will be _very_ long, even for small regexes.

  inline static int trace_depth = 0;

  template<StringParam type, typename P>
  static void print_bar(const char* a, const char* b, const char* suffix) {
    //printf("{%-20.20s}   ", a);
    printf("|");
    print_flat(a, b, 20);
    printf("| ");
    for (int i = 0; i < trace_depth; i++) printf("|  ");
    printf("%s %s\n", type.str_val, suffix);
  }

  template<StringParam type, typename P>
  const char* trace(const char* a, const char* b) {
    if (!a || a == b) return nullptr;
    print_bar<type, P>(a, b, "?");
    trace_depth++;
    auto end = P::match(this, a, b);
    trace_depth--;
    print_bar<type, P>(a, b, end ? "OK" : "X");
    return end;
  }

  //----------------------------------------

  NodeBase* top_head = nullptr;
  NodeBase* top_tail = nullptr;
};

//------------------------------------------------------------------------------
// Matcheroni's default rewind callback does nothing, but if we provide a
// specialized version of it Matcheroni will call it as needed.

template<>
void matcheroni::parser_rewind(void* ctx, const char* a, const char* b) {
  ((Parser*)ctx)->rewind(a);
}

//------------------------------------------------------------------------------
// To build a parse tree, we wrap the patterns we want to create nodes for
// in a Capture<> matcher that will invoke our node factory. We can also wrap
// them in a Trace<> matcher if we want to debug our patterns.

#ifdef TRACE

template<StringParam type, typename P>
using Trace = Ref<&Parser::trace<type, P>>,

template<StringParam type, typename P>
using Capture = JsonNodeFactory<type, Trace<type, P>>;

#else

template<StringParam field_name, typename pattern, typename NodeType>
struct CaptureNode {

  static const char* match(void* ctx, const char* a, const char* b) {
    Parser* parser = (Parser*)ctx;

    if (auto n = parser->node_factory<NodeType, pattern>(a, b)) {
      n->field_name = field_name.str_val;
      return n->b;
    }
    else {
      return nullptr;
    }
  }
};

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

using member = Seq<Capture<"key", string>, ws, Atom<':'>, ws, Capture<"value", value>>;

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

      char* text = nullptr;
      int text_size = 0;
      read(path, text, text_size);
      byte_accum += text_size;
      //printf("Read %d bytes\n", text_size);

      const char* text_a = (const char*)text;
      const char* text_b = text_a + text_size;

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
          for (JsonNode* n = (JsonNode*)parser->top_head; n; n = (JsonNode*)n->next) {
            n->print_tree();
          }
        }

        if (*end != 0) {
          printf("Leftover text: |%s|\n", end);
        }
      }

      delete [] text;


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
