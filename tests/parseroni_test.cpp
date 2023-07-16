#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <string>

using namespace matcheroni;

struct TestNode : public TextNode {
  using TextNode::TextNode;

  TestNode* node_prev()  { return (TestNode*)TextNode::node_prev(); }
  TestNode* node_next()  { return (TestNode*)TextNode::node_next(); }

  const TestNode* node_prev() const { return (const TestNode*)TextNode::node_prev(); }
  const TestNode* node_next() const { return (const TestNode*)TextNode::node_next(); }

  TestNode* child_head() { return (TestNode*)TextNode::child_head(); }
  TestNode* child_tail() { return (TestNode*)TextNode::child_tail(); }

  const TestNode* child_head() const { return (const TestNode*)TextNode::child_head(); }
  const TestNode* child_tail() const { return (const TestNode*)TextNode::child_tail(); }

  void dump_tree(std::string& out) {
    if (strcmp(match_name, "atom") == 0) {
      for (auto c = span.a; c < span.b; c++) out.push_back(*c);
    } else if (strcmp(match_name, "list") == 0) {
      out.push_back('(');
      for (auto c = child_head(); c; c = c->node_next()) {
        ((TestNode*)c)->dump_tree(out);
        if (c->node_next()) out.push_back(',');
      }
      out.push_back(')');
    } else {
      matcheroni_assert(false);
    }
  }
};

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

//----------------------------------------

template<typename Context>
void check_hash(const Context& context, uint64_t hash_a) {
  uint64_t hash_b = hash_context(&context);
  printf("Expected hash 0x%016lx\n", hash_a);
  printf("Actual hash   0x%016lx\n", hash_b);
  matcheroni_assert(hash_a == hash_b && "bad hash");
}

//----------------------------------------

void test_basic() {
  printf("test_basic()\n");
  TextContext context;
  cspan span;
  cspan tail;

  {
    // Check than we can round-trip a s-expression
    std::string text = "(abcd,efgh,(ab),(a,(bc,de)),ghijk)";
    context.reset();
    span = to_span(text);
    tail = TinyLisp::match(&context, span);
    matcheroni_assert(tail.is_valid() && tail == "");

    printf("Round-trip s-expression:\n");
    std::string dump;
    ((TestNode*)context.top_head())->dump_tree(dump);
    printf("Old : %s\n", text.c_str());
    printf("New : %s\n", dump.c_str());
    matcheroni_assert(text == dump && "Mismatch!");
    printf("\n");

    print_context(&context);
    check_hash(context, 0x7073c4e1b84277f0);
  }

  context.reset();
  span = to_span("((((a))))");
  tail = TinyLisp::match(&context, span);
  matcheroni_assert(tail.is_valid() && tail == "");

  context.reset();
  span = to_span("(((())))");
  tail = TinyLisp::match(&context, span);
  matcheroni_assert(tail.is_valid() && tail == "");

  context.reset();
  span = to_span("(((()))(");
  tail = TinyLisp::match(&context, span);
  matcheroni_assert(!tail.is_valid() && std::string(tail.b) == "(");

  printf("test_basic() end\n\n");
}

//------------------------------------------------------------------------------

void test_rewind() {
  printf("test_rewind()\n");

  std::string text = "abcdef";
  auto span = to_span(text);

  using pattern =
  Oneof<
    Seq<
      Capture<"a", Atom<'a'>, TestNode>,
      Capture<"b", Atom<'b'>, TestNode>,
      Capture<"c", Atom<'c'>, TestNode>,
      Capture<"d", Atom<'d'>, TestNode>,
      Capture<"e", Atom<'e'>, TestNode>,

      Capture<"g", Atom<'g'>, TestNode>
    >,
    Capture<"lit", Lit<"abcdef">, TestNode>
  >;

  TextContext context;
  auto tail = pattern::match(&context, span);

  print_match(span, span - tail);
  print_context(&context);

  //check_hash(context, 0x2850a87bce45242a);

  printf("sizeof(ParseNode) %ld\n", sizeof(TestNode));
  printf("node count %ld\n", context.top_head()->node_count());
  printf("constructor calls %ld\n", TextNode::constructor_calls);
  printf("destructor calls %ld\n", TextNode::destructor_calls);
  printf("max size %d\n", SlabAlloc::slabs().max_size);
  printf("current size %d\n", SlabAlloc::slabs().current_size);

  //matcheroni_assert(SlabAlloc::slabs().current_size == sizeof(ParseNode));

  printf("test_rewind() end\n\n");
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
    TestNode,
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

  TextContext context;
  auto tail = BeginEndTest::match(&context, text);

  print_match(text, text - tail);
  print_context(&context);

  check_hash(context, 0x401403cbefc2cba9);

  printf("sizeof(ParseNode) %ld\n", sizeof(TestNode));
  printf("node count %ld\n", context.top_head()->node_count());
  printf("constructor calls %ld\n", TextNode::constructor_calls);
  printf("destructor calls %ld\n", TextNode::destructor_calls);
  printf("max size %d\n", SlabAlloc::slabs().max_size);
  printf("current size %d\n", SlabAlloc::slabs().current_size);

  printf("test_begin_end() end\n\n");
}

//------------------------------------------------------------------------------
// This matcher matches nested square brackets with a single letter in the
// middle, but it does so in a pathologically-horrible way that requires
// partially matching and then backtracking through a bunch of cases due to a
// mismatched suffix.

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
  printf("test_pathological()\n");
  TextContext context;
  cspan span;
  cspan tail;

  // We expect 137257 constructor calls and 137250 destructor calls for this
  // pattern.
  std::string text = "[[[[[[a]]]]]]";

  span = to_span(text);
  tail = Pathological::match(&context, span);
  matcheroni_assert(tail.is_valid() && "pathological tree invalid");

  // Tree should be
  // {[[[[[[a]]]]]]       } *none
  // {[[[[[a]]]]]         }  |-none
  // {[[[[a]]]]           }  | |-none
  // {[[[a]]]             }  | | |-none
  // {[[a]]               }  | | | |-none
  // {[a]                 }  | | | | |-none
  // {a                   }  | | | | | |-atom

  matcheroni_assert(context.top_head()->node_count() == 7);
  print_context(&context);
  check_hash(context, 0x07a37a832d506209);

  matcheroni_assert(TextNode::constructor_calls == 137257);
  matcheroni_assert(TextNode::destructor_calls  == 137250);

  matcheroni_assert(SlabAlloc::slabs().max_size == sizeof(TestNode) * 7);
  matcheroni_assert(SlabAlloc::slabs().current_size == sizeof(TestNode) * 7);

  //printf("sizeof(ParseNode) %ld\n", sizeof(ParseNode));
  //printf("node count %ld\n", context.top_head()->node_count());
  //printf("constructor calls %ld\n", TextNode::constructor_calls);
  //printf("destructor calls %ld\n", TextNode::destructor_calls);
  //printf("max size %d\n", SlabAlloc::slabs().max_size);
  //printf("current size %d\n", SlabAlloc::slabs().current_size);

  printf("test_pathological() end\n\n");
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Parseroni tests\n");

  //test_basic();
  //test_rewind();
  //test_begin_end();
  test_pathological();

  printf("All tests pass!\n");
  return 0;
}

//------------------------------------------------------------------------------
