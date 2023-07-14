#include "matcheroni/Parseroni.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

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

#define TEST(A, ...)                                                \
  if (!(A)) {                                                        \
    fail_count++;                                                    \
    printf("\n");                                                    \
    printf("TEST(" #A ") fail: @ %s/%s:%d", __FILE__, __FUNCTION__, \
           __LINE__);                                                \
    printf("\n  " __VA_ARGS__);                                      \
    printf("\n");                                                    \
  }

//------------------------------------------------------------------------------

struct BeginEndTest {
  static cspan match(void* ctx, cspan s) {
    return Oneof<atom, list>::match(ctx, s);
  }

  using ws = Any<Atom<' ', '\n', '\r', '\t'>>;

  template<typename P>
  using suffixed =
  CaptureBegin<
    P,
    Opt<
      CaptureEnd<"plus", Atom<'+'>, TestNode>,
      CaptureEnd<"star", Atom<'*'>, TestNode>,
      CaptureEnd<"opt",  Atom<'?'>, TestNode>
    >
  >;

  using atom =
  suffixed<Capture<
    "atom",
    Some<Range<'a','z'>, Range<'A','Z'>, Range<'0','9'>>,
    TestNode
  >>;

  using car = Opt<Ref<match>>;
  using cdr = Any<Seq<ws, Atom<','>, ws, Ref<match>>>;

  using list =
  suffixed<Capture<
    "list",
    Seq<Atom<'['>, ws, car, cdr, ws, Atom<']'>>,
    TestNode
  >>;
};

void test_begin_end() {
  printf("test_begin_end()\n");

  auto text = to_span("[ [abc,ab?,cdb+] , [a,b,c*,d,e,f] ]");

  Context c;
  auto tail = BeginEndTest::match(&c, text);

  if (tail.is_valid()) {
    print_match(text, text - tail);
    if (c.top_head) print_tree(c.top_head);
  }
  else {
    printf("invalid\n");
  }

  printf("sizeof(TestNode) %ld\n", sizeof(TestNode));
  printf("node count %ld\n", c.top_head->node_count());
  printf("constructor calls %ld\n", NodeBase::constructor_calls);
  printf("destructor calls %ld\n", NodeBase::destructor_calls);
  printf("max size %d\n", NodeBase::slabs.max_size);
  printf("current size %d\n", NodeBase::slabs.current_size);

  printf("test_begin_end() done\n");
  printf("\n");
}

//------------------------------------------------------------------------------

#if 0
struct Pathological {
  static cspan match(void* ctx, cspan s) {
    return pattern::match(ctx, s);
  }

  using pattern =
  Oneof<
    Capture<"plus",  Seq<Atom<'['>, Ref<match>, Atom<']'>, Atom<'+'>>, TestNode>,
    Capture<"minus", Seq<Atom<'['>, Ref<match>, Atom<']'>, Atom<'-'>>, TestNode>,
    Capture<"star",  Seq<Atom<'['>, Ref<match>, Atom<']'>, Atom<'*'>>, TestNode>,
    Capture<"slash", Seq<Atom<'['>, Ref<match>, Atom<']'>, Atom<'/'>>, TestNode>,
    Capture<"opt",   Seq<Atom<'['>, Ref<match>, Atom<']'>, Atom<'?'>>, TestNode>,
    Capture<"eq",    Seq<Atom<'['>, Ref<match>, Atom<']'>, Atom<'='>>, TestNode>,
    Capture<"none",  Seq<Atom<'['>, Ref<match>, Atom<']'>>, TestNode>,
    Capture<"atom",  Range<'a','z'>, TestNode>
  >;
};

void test_pathological() {
  Context c;
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

  c.reset();
  span = to_span(text);
  tail = Pathological::match(&c, span);
  TEST(tail.is_valid(), "pathological tree invalid");
  print_tree(c.top_head);
  printf("sizeof(TestNode) %ld\n", sizeof(TestNode));
  printf("node count %ld\n", c.top_head->node_count());
  printf("constructor calls %ld\n", NodeBase::constructor_calls);
  printf("destructor calls %ld\n", NodeBase::destructor_calls);
  printf("max size %d\n", NodeBase::slabs.max_size);
  printf("current size %d\n", NodeBase::slabs.current_size);
}
#endif

//------------------------------------------------------------------------------
// A whole s-expression parser in ~10 lines of code. :D

struct TinyLisp {
  static cspan match(void* ctx, cspan s) {
    return Oneof<
      Capture<"atom", atom, TestNode>,
      Capture<"list", list, TestNode>
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
  Context c;
  cspan span;
  cspan tail;

  {
    // Check than we can round-trip a s-expression
    std::string text = "(abcd,efgh,(ab),(a,(bc,de)),ghijk)";
    c.reset();
    span = to_span(text);
    tail = TinyLisp::match(&c, span);
    TEST(tail.is_valid() && tail == "");
    std::string dump;
    ((TestNode*)c.top_head)->dump_tree(dump);
    TEST(text == dump, "Round trip s-expression");
  }

  c.reset();
  span = to_span("((((a))))");
  tail = TinyLisp::match(&c, span);
  TEST(tail.is_valid() && tail == "");

  c.reset();
  span = to_span("(((())))");
  tail = TinyLisp::match(&c, span);
  TEST(tail.is_valid() && tail == "");

  c.reset();
  span = to_span("(((()))(");
  tail = TinyLisp::match(&c, span);
  TEST(!tail.is_valid() && tail == "(");
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Parseroni tests\n");

  //test_basic();
  test_begin_end();
  //test_backcap();
  //test_pathological();

  if (!fail_count) {
    printf("All tests pass!\n");
    return 0;
  } else {
    printf("Failed %d tests!\n", fail_count);
    return -1;
  }
}

//------------------------------------------------------------------------------
