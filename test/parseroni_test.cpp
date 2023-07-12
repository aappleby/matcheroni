#include "matcheroni/Parseroni.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "matcheroni/Matcheroni.hpp"

using namespace matcheroni;

struct TestNode : public NodeBase {
  using NodeBase::NodeBase;

  void dump_tree(std::string& out) {
    if (strcmp(match_name, "atom") == 0) {
      for (auto c = span.a; c < span.b; c++) out.push_back(*c);
    } else if (strcmp(match_name, "list") == 0) {
      out.push_back('(');
      for (auto c = (TestNode*)child_head; c; c = (TestNode*)c->node_next) {
        c->dump_tree(out);
        if (c->node_next) out.push_back(',');
      }
      out.push_back(')');
    } else {
      assert(false);
    }
  }
};

//------------------------------------------------------------------------------

static int fail_count = 0;

#define CHECK(A, ...)                                                \
  if (!(A)) {                                                        \
    fail_count++;                                                    \
    printf("\n");                                                    \
    printf("CHECK(" #A ") fail: @ %s/%s:%d", __FILE__, __FUNCTION__, \
           __LINE__);                                                \
    printf("\n  " __VA_ARGS__);                                      \
    printf("\n");                                                    \
  }

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// A whole s-expression parser in ~10 lines of code. :D

cspan match_element(void* ctx, cspan s);
using element = Ref<match_element>;

using ws = Any<Atom<' ', '\n', '\r', '\t'>>;
using atom = Some<NotAtom<'(', ')', ' ', '\n', '\r', '\t', ','>>;
using car = Opt<element>;
using cdr = Any<Seq<ws, Atom<','>, ws, element>>;
using list = Seq<Atom<'('>, ws, car, cdr, ws, Atom<')'>>;

cspan match_element(void* ctx, cspan s) {
  return Oneof<
    CaptureNamed<"atom", atom, TestNode>,
    CaptureNamed<"list", list, TestNode>
  >::match(ctx, s);
}

//------------------------------------------------------------------------------

void test_basic() {
  Parser p;
  cspan span;
  cspan tail;


  {
    // Check than we can round-trip a s-expression
    std::string text = "(abcd,efgh,(ab),(a,(bc,de)),ghijk)";
    p.reset();
    span = to_span(text);
    tail = element::match(&p, span);
    CHECK(tail.valid() && tail == "");
    std::string dump;
    ((TestNode*)p.top_head)->dump_tree(dump);
    CHECK(text == dump, "Round trip s-expression");
  }

  p.reset();
  span = to_span("((((a))))");
  tail = element::match(&p, span);
  CHECK(tail.valid() && tail == "");

  p.reset();
  span = to_span("(((())))");
  tail = element::match(&p, span);
  CHECK(tail.valid() && tail == "");

  p.reset();
  span = to_span("(((()))(");
  tail = element::match(&p, span);
  CHECK(!tail.valid() && tail == "(");
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Parseroni tests\n");

  test_basic();

  if (!fail_count) {
    printf("All tests pass!\n");
    return 0;
  } else {
    printf("Failed %d tests!\n", fail_count);
    return -1;
  }
}

//------------------------------------------------------------------------------
