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

struct BackcapTest {
  static cspan match(void* ctx, cspan s) {
    return pattern::match(ctx, s);
  }

  using pattern =
  Oneof<
    CaptureNamed<"atom", Range<'a','z'>, TestNode>,
    Seq<
      CaptureNamed<"match", Seq<Atom<'['>, Ref<match>, Atom<']'>>, TestNode>,
      Oneof<
        BackCap<1, "plus",  Atom<'+'>, TestNode>,
        BackCap<1, "minus", Atom<'-'>, TestNode>,
        BackCap<1, "star",  Atom<'*'>, TestNode>,
        BackCap<1, "slash", Atom<'/'>, TestNode>,
        BackCap<1, "opt",   Atom<'?'>, TestNode>,
        BackCap<1, "eq",    Atom<'='>, TestNode>,
        BackCap<1, "none",  Nothing,   TestNode>
      >
    >
  >;
};

void test_backcap() {
  Context p;
  cspan span;
  cspan tail;

  // "a"   - cap 8 create 1
  // "[a]" - cap 63 create 8
  // "[[a]]" - cap 448 create 57
  // "[[[a]]]" - cap 3143 create 400
  // "[[[[a]]]]" - cap 22008 create 2801
  // "[[[[[a]]]]]" - cap 154063 create 19608
  // "[[[[[[a]]]]]]" - cap 1078448 create 137257

  //std::string text = "[[[a]]]";
  std::string text = "[[[[[[a]]]]]]";

  p.reset();
  span = to_span(text);
  tail = BackcapTest::match(&p, span);
  CHECK(tail.is_valid(), "pathological tree invalid");
  print_tree(p.top_head);
  printf("sizeof(TestNode) %ld\n", sizeof(TestNode));
  printf("node count %ld\n", p.top_head->node_count());
  printf("constructor calls %ld\n", NodeBase::constructor_calls);
  printf("destructor calls %ld\n", NodeBase::destructor_calls);
  printf("max size %d\n", NodeBase::slabs.max_size);
  printf("current size %d\n", NodeBase::slabs.current_size);
}

//------------------------------------------------------------------------------

struct Pathological {
  static cspan match(void* ctx, cspan s) {
    return pattern::match(ctx, s);
  }

  using pattern =
  Oneof<
    CaptureNamed<"plus",  Seq<Atom<'['>, Ref<match>, Atom<']'>, Atom<'+'>>, TestNode>,
    CaptureNamed<"minus", Seq<Atom<'['>, Ref<match>, Atom<']'>, Atom<'-'>>, TestNode>,
    CaptureNamed<"star",  Seq<Atom<'['>, Ref<match>, Atom<']'>, Atom<'*'>>, TestNode>,
    CaptureNamed<"slash", Seq<Atom<'['>, Ref<match>, Atom<']'>, Atom<'/'>>, TestNode>,
    CaptureNamed<"opt",   Seq<Atom<'['>, Ref<match>, Atom<']'>, Atom<'?'>>, TestNode>,
    CaptureNamed<"eq",    Seq<Atom<'['>, Ref<match>, Atom<']'>, Atom<'='>>, TestNode>,
    CaptureNamed<"none",  Seq<Atom<'['>, Ref<match>, Atom<']'>>, TestNode>,
    CaptureNamed<"atom",  Range<'a','z'>, TestNode>
  >;
};

void test_pathological() {
  Context p;
  cspan span;
  cspan tail;

  // "a"   - cap 8 create 1
  // "[a]" - cap 63 create 8
  // "[[a]]" - cap 448 create 57
  // "[[[a]]]" - cap 3143 create 400
  // "[[[[a]]]]" - cap 22008 create 2801
  // "[[[[[a]]]]]" - cap 154063 create 19608
  // "[[[[[[a]]]]]]" - cap 1078448 create 137257

  //std::string text = "[[[a]]]";
  std::string text = "[[[[[[a]]]]]]";

  p.reset();
  span = to_span(text);
  tail = Pathological::match(&p, span);
  CHECK(tail.is_valid(), "pathological tree invalid");
  print_tree(p.top_head);
  printf("sizeof(TestNode) %ld\n", sizeof(TestNode));
  printf("node count %ld\n", p.top_head->node_count());
  printf("constructor calls %ld\n", NodeBase::constructor_calls);
  printf("destructor calls %ld\n", NodeBase::destructor_calls);
  printf("max size %d\n", NodeBase::slabs.max_size);
  printf("current size %d\n", NodeBase::slabs.current_size);
}

//------------------------------------------------------------------------------
// A whole s-expression parser in ~10 lines of code. :D

struct TinyLisp {
  static cspan match(void* ctx, cspan s) {
    return Oneof<
      CaptureNamed<"atom", atom, TestNode>,
      CaptureNamed<"list", list, TestNode>
    >::match(ctx, s);
  }

  using ws = Any<Atom<' ', '\n', '\r', '\t'>>;
  using atom = Some<NotAtom<'(', ')', ' ', '\n', '\r', '\t', ','>>;
  using car = Opt<Ref<match>>;
  using cdr = Any<Seq<ws, Atom<','>, ws, Ref<match>>>;
  using list = Seq<Atom<'('>, ws, car, cdr, ws, Atom<')'>>;
};

//------------------------------------------------------------------------------

void test_basic() {
  Context p;
  cspan span;
  cspan tail;

  {
    // Check than we can round-trip a s-expression
    std::string text = "(abcd,efgh,(ab),(a,(bc,de)),ghijk)";
    p.reset();
    span = to_span(text);
    tail = TinyLisp::match(&p, span);
    CHECK(tail.is_valid() && tail == "");
    std::string dump;
    ((TestNode*)p.top_head)->dump_tree(dump);
    CHECK(text == dump, "Round trip s-expression");
  }

  p.reset();
  span = to_span("((((a))))");
  tail = TinyLisp::match(&p, span);
  CHECK(tail.is_valid() && tail == "");

  p.reset();
  span = to_span("(((())))");
  tail = TinyLisp::match(&p, span);
  CHECK(tail.is_valid() && tail == "");

  p.reset();
  span = to_span("(((()))(");
  tail = TinyLisp::match(&p, span);
  CHECK(!tail.is_valid() && tail == "(");
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Parseroni tests\n");

  //test_basic();
  test_backcap();
  test_pathological();

  if (!fail_count) {
    printf("All tests pass!\n");
    return 0;
  } else {
    printf("Failed %d tests!\n", fail_count);
    return -1;
  }
}

//------------------------------------------------------------------------------
