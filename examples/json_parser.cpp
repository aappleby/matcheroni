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
#include "matcheroni/Matcheroni.hpp"
#include <sys/stat.h>
#include <chrono>

using namespace matcheroni;

//------------------------------------------------------------------------------

void read(const char* path, char*& text_out, int& size_out) {
  //auto size = std::filesystem::file_size(path);
  //text.resize(size);

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

struct SlabAlloc {

  SlabAlloc() {
    top_slab = new uint8_t[slab_size];
    slab_cursor = 0;
    current_size = 0;
    max_size = 0;
  }

  void free() {
    for (int i = 0; i < old_slab_count; i++) delete [] old_slabs[i];
    for (int i = 0; i < new_slab_count; i++) delete [] new_slabs[i];
    delete [] top_slab;
  }

  void reset() {
    while (old_slab_count) {
      new_slabs[new_slab_count++] = old_slabs[--old_slab_count];
    }
    slab_cursor = 0;
    current_size = 0;
  }

  void* bump(size_t size) {
    if (slab_cursor + size > slab_size) {
      old_slabs[old_slab_count++] = top_slab;
      if (new_slab_count == 0) {
        top_slab = new uint8_t[slab_size];
      }
      else {
        top_slab = new_slabs[--new_slab_count];
      }
      slab_cursor = 0;
    }

    auto result = top_slab + slab_cursor;
    slab_cursor += size;

    current_size += size;
    if (current_size > max_size) max_size = current_size;

    return result;
  }

  // slab size is 1 hugepage. seems to work ok.
  static constexpr size_t slab_size = 2*1024*1024;

  uint8_t* old_slabs[1024];
  int old_slab_count = 0;
  uint8_t* new_slabs[1024];
  int new_slab_count = 0;

  uint8_t* top_slab;
  size_t   slab_cursor;
  size_t   current_size;
  size_t   max_size;
};

SlabAlloc slabs;

//------------------------------------------------------------------------------


// Uncomment this to print a full trace of the regex matching process. Note -
// the trace will be _very_ long, even for small regexes.
//#define TRACE

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
// Our parse node for this example is pretty trivial - a type name, the
// endpoints of the matched text, and a list of child nodes.

struct Node {

  static void* operator new(std::size_t size)   { return slabs.bump(size); }
  static void* operator new[](std::size_t size) { return slabs.bump(size); }
  static void  operator delete(void*)           { }
  static void  operator delete[](void*)         { }

  Node() {
    type = nullptr;
    a = nullptr;
    b = nullptr;

    prev = nullptr;
    next = nullptr;

    head = nullptr;
    tail = nullptr;
  }

  const char* type;
  const char* a;
  const char* b;

  Node* prev = nullptr;
  Node* next = nullptr;

  Node* head = nullptr;
  Node* tail = nullptr;

  // Prints a text representation of the parse tree.
  void print_tree(int depth = 0) {
    //if (depth > 3) return;

    // Print the node's matched text, with a "..." if it doesn't fit in 20
    // characters.
    print_flat(a, b, 20);

    // Print out the name of the type name of the node with indentation.

    printf("   ");
    for (int i = 0; i < depth; i++) printf(i == depth-1 ? "|--" : "|  ");
    printf("%s\n", type);
    for (auto c = head; c; c = c->next) c->print_tree(depth+1);
  }
};

//------------------------------------------------------------------------------
// To convert our pattern matches to parse nodes, we create a Factory<> matcher
// that constructs a new NodeType() for a successful match, attaches any
// sub-nodes to it, and places it on a node stack.

// If this were a larger application, we would keep the node stack inside a
// match context object passed in via 'ctx', but a global is fine for now.

Node* top_head;
Node* top_tail;

template<StringParam type, typename P, typename NodeType>
struct Factory {
  static const char* match(void* ctx, const char* a, const char* b) {
    if (!a || a == b) return nullptr;

    auto old_top_tail = top_tail;
    auto end = P::match(ctx, a, b);
    auto new_top_tail = top_tail;

    if (!end) return nullptr;

    auto new_node = new NodeType();
    new_node->type = type.str_val;
    new_node->a = a;
    new_node->b = end;

    if (new_top_tail != old_top_tail) {

      if (old_top_tail) {
        // Attach all nodes after old_top_tail to the new node.
        new_node->head = old_top_tail->next;
        new_node->tail = new_top_tail;
        old_top_tail->next->prev = nullptr;
        old_top_tail->next = nullptr;
        top_tail = old_top_tail;
      }
      else {
        // We are the new top node, assimilate all the current nodes
        new_node->head = top_head;
        new_node->tail = top_tail;
        top_head = nullptr;
        top_tail = nullptr;
      }
    }

    if (top_tail) {
      top_tail->next = new_node;
      new_node->prev = top_tail;
      new_node->next = nullptr;

      top_tail = new_node;
    }
    else {
      new_node->prev = nullptr;
      new_node->next = nullptr;

      top_head = new_node;
      top_tail = new_node;
    }

    /*
    printf("========================================\n");
    for (auto c = top_head; c; c = c->next) c->print_tree();
    printf("\n");
    */

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
  while(top_tail && top_tail->b > a) {
    auto dead = top_tail;
    top_tail = top_tail->prev;
    delete dead;
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

  static void print_bar(const char* a, const char* b, const char* suffix) {
    //printf("{%-20.20s}   ", a);
    printf("|");
    print_flat(a, b, 20);
    printf("| ");
    for (int i = 0; i < trace_depth; i++) printf("|  ");
    printf("%s %s\n", type.str_val, suffix);
  }

  static const char* match(void* ctx, const char* a, const char* b) {
    if (!a || a == b) return nullptr;
    print_bar(a, b, "?");
    trace_depth++;
    auto end = P::match(ctx, a, b);
    trace_depth--;
    print_bar(a, b, end ? "OK" : "X");
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
  Capture<"key", string>, ws,
  Atom<':'>, ws,
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
  Atom<'['>, ws,
  Opt<comma_separated<value>>, ws,
  Atom<']'>
>;

const char* match_value(void* ctx, const char* a, const char* b) {
  using value =
  Oneof<
    Capture<"object",  object>,
    Capture<"array",   array>,
    Capture<"string",  string>,
    Capture<"number",  number>,
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
    "../nativejson-benchmark/data/canada.json",
    "../nativejson-benchmark/data/citm_catalog.json",
    "../nativejson-benchmark/data/twitter.json",
  };

  double time_accum = 0;

  const int reps = 100;
  for (int rep = 0; rep < reps; rep++) {
    for (auto path : paths) {

      //printf("//----------------------------------------\n");
      //printf("// Parsing %s\n", path);

      char* text = nullptr;
      int text_size = 0;
      read(path, text, text_size);
      //printf("Read %d bytes\n", text_size);

      assert(top_head == nullptr);
      assert(top_tail == nullptr);

      const char* text_a = (const char*)text;
      const char* text_b = text_a + text_size;

      double time = -timestamp_ms();
      const char* end = json::match(nullptr, text_a, text_b);
      time += timestamp_ms();
      time_accum += time;
      //printf("Matching took %f msec\n", time);

      // If everything went well, our node stack should now have a sequence of
      // parse nodes in it.

      if (!end) {
        printf("Our matcher could not match anything!\n");
      }
      else if (top_head == nullptr) {
        printf("No parse nodes created!\n");
      }
      else {
        //printf("Parse tree:\n");
        //for (auto n = top_head; n; n = n->next) {
        //  n->print_tree();
        //}

        if (*end != 0) {
          printf("Leftover text: |%s|\n", end);
        }
      }

      //printf("Slab current %ld\n", slabs.current_size);
      //printf("Slab max     %ld\n", slabs.max_size);
      //printf("Node count   %ld\n", slabs.current_size / sizeof(Node));
      //printf("\n");

      delete [] text;
      top_head = top_tail = nullptr;
      slabs.reset();
    }
  }

  printf("time_accum %f\n", time_accum);
  slabs.free();

  return 0;
}

//------------------------------------------------------------------------------
