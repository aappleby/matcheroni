#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

#include <stdio.h>
#include <string.h>
#include <string>

using namespace matcheroni;

//------------------------------------------------------------------------------

struct TestNode : public TextNode, public utils::InstanceCounter<TestNode> {
  using TextNode::TextNode;

  TestNode* node_prev()  { return (TestNode*)_node_prev; }
  TestNode* node_next()  { return (TestNode*)_node_next; }

  const TestNode* node_prev() const { return (const TestNode*)_node_prev; }
  const TestNode* node_next() const { return (const TestNode*)_node_next; }

  TestNode* child_head() { return (TestNode*)_child_head; }
  TestNode* child_tail() { return (TestNode*)_child_tail; }

  const TestNode* child_head() const { return (const TestNode*)_child_head; }
  const TestNode* child_tail() const { return (const TestNode*)_child_tail; }
};

//----------------------------------------

void sexp_to_string(TestNode* n, std::string& out) {
  if (strcmp(n->match_name, "atom") == 0) {
    for (auto c = n->span.begin; c < n->span.end; c++) out.push_back(*c);
  } else if (strcmp(n->match_name, "list") == 0) {
    out.push_back('(');
    for (auto c = n->child_head(); c; c = c->node_next()) {
      sexp_to_string(c, out);
      if (c->node_next()) out.push_back(',');
    }
    out.push_back(')');
  } else {
    matcheroni_assert(false);
  }
}

//----------------------------------------

template<typename context>
void check_hash(context& ctx, uint64_t hash_a) {
  uint64_t hash_b = utils::hash_context(ctx);
  printf("Expected hash 0x%016lx\n", hash_a);
  printf("Actual hash   0x%016lx\n", hash_b);
  matcheroni_assert(hash_a == hash_b && "bad hash");
}

//----------------------------------------

void reset_everything() {
  LinearAlloc::inst().reset();
  utils::InstanceCounter<TestNode>::reset();
  matcheroni_assert(utils::InstanceCounter<TestNode>::live == 0);
  matcheroni_assert(utils::InstanceCounter<TestNode>::dead == 0);
  matcheroni_assert(LinearAlloc::inst().current_size == 0);
  matcheroni_assert(LinearAlloc::inst().max_size == 0);
}

//------------------------------------------------------------------------------
// A mini s-expression parser in ~10 lines of code. :D

struct SExpression {
  static TextSpan match(TextNodeContext& ctx, TextSpan body) {
    return Oneof<
      Capture<"atom", atom, TestNode>,
      Capture<"list", list, TestNode>
    >::match(ctx, body);
  }

  using space = Some<Atom<' ', '\n', '\r', '\t'>>;
  using atom  = Some<Range<'0','9','a','z','A','Z', '_', '_'>>;
  using car   = Ref<match>;
  using cdr   = Some<Seq<Atom<','>, Any<space>, Ref<match>>>;
  using list  = Seq<Atom<'('>, Opt<space>, Opt<car>, Opt<space>, Opt<cdr>, Opt<space>, Atom<')'>>;
};

//----------------------------------------

void test_basic() {
  printf("test_basic()\n");
  reset_everything();

  {
    // Check than we can round-trip a s-expression

    std::string expression = "(abcd,efgh,(ab),(a,(bc,de)),ghijk)";

    TextNodeContext ctx;
    ctx.reset();
    auto text = utils::to_span(expression);
    auto tail = SExpression::match(ctx, text);
    matcheroni_assert(tail.is_valid() && tail == "");

    utils::print_summary(text, tail, ctx, 50);

    printf("Round-trip s-expression:\n");
    std::string new_text;
    sexp_to_string((TestNode*)ctx.top_head(), new_text);
    printf("Old : %s\n", text.begin);
    printf("New : %s\n", new_text.c_str());
    matcheroni_assert(expression == new_text && "Mismatch!");
    printf("\n");

    utils::print_summary(text, tail, ctx, 50);

    check_hash(ctx, 0x7073c4e1b84277f0);

    matcheroni_assert(LinearAlloc::inst().max_size == sizeof(TestNode) * 11);
    matcheroni_assert(LinearAlloc::inst().current_size == sizeof(TestNode) * 11);
    matcheroni_assert(utils::InstanceCounter<TestNode>::live == 11);
    matcheroni_assert(utils::InstanceCounter<TestNode>::dead == 0);
  }

  TextNodeContext ctx;
  TextSpan span;
  TextSpan tail;

  ctx.reset();
  span = utils::to_span("((((a))))");
  tail = SExpression::match(ctx, span);
  matcheroni_assert(tail.is_valid() && tail == "");

  ctx.reset();
  span = utils::to_span("(((())))");
  tail = SExpression::match(ctx, span);
  matcheroni_assert(tail.is_valid() && tail == "");

  ctx.reset();
  span = utils::to_span("(((()))(");
  tail = SExpression::match(ctx, span);
  matcheroni_assert(!tail.is_valid() && std::string(tail.end) == "(");

  printf("test_basic() end\n\n");
}

//------------------------------------------------------------------------------

void test_rewind() {
  printf("test_rewind()\n");
  reset_everything();

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

  TextNodeContext ctx;

  auto text = utils::to_span("abcdef");
  auto tail = pattern::match(ctx, text);

  utils::print_summary(text, tail, ctx, 50);
  check_hash(ctx, 0x2850a87bce45242a);

  matcheroni_assert(utils::InstanceCounter<TestNode>::live == 1);
  matcheroni_assert(utils::InstanceCounter<TestNode>::dead == 5);
  matcheroni_assert(LinearAlloc::inst().current_size == sizeof(TestNode) * 1);
  matcheroni_assert(LinearAlloc::inst().max_size == sizeof(TestNode) * 5);

  printf("test_rewind() end\n\n");
}

//------------------------------------------------------------------------------

struct BeginEndTest {

  static TextSpan match(TextNodeContext& ctx, TextSpan body) {
    return Oneof<
      suffixed<Capture<"atom", atom, TestNode>>,
      suffixed<Capture<"list", list, TestNode>>
    >::match(ctx, body);
  }

  // suffixed<> wraps the previously-matched node in another node if it has
  // a suffix. Slightly brain-hurty.
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

  using space = Some<Atom<' ', '\n', '\r', '\t'>>;
  using atom  = Some<Range<'0','9','a','f','A','F'>>;
  using car   = Opt<Ref<match>>;
  using cdr   = Any<Seq<Atom<','>, Any<space>, Ref<match>>>;
  using list  = Seq<Atom<'['>, Any<space>, car, Any<space>, cdr, Any<space>, Atom<']'>>;
};

//------------------------------------------------------------------------------

void test_begin_end() {
  printf("test_begin_end()\n");
  reset_everything();

  TextNodeContext ctx;

  auto text = utils::to_span("[ [abc,ab?,cdb+] , [a,b,c*,d,e,f] ]");
  auto tail = BeginEndTest::match(ctx, text);

  utils::print_summary(text, tail, ctx, 50);
  check_hash(ctx, 0x401403cbefc2cba9);

  matcheroni_assert(utils::InstanceCounter<TestNode>::live == 15);
  matcheroni_assert(utils::InstanceCounter<TestNode>::dead == 0);
  matcheroni_assert(LinearAlloc::inst().max_size == sizeof(TestNode) * 15);
  matcheroni_assert(LinearAlloc::inst().current_size == sizeof(TestNode) * 15);

  printf("test_begin_end() end\n\n");
}

//------------------------------------------------------------------------------
// This matcher matches nested square brackets with a single letter in the
// middle, but it does so in a pathologically-horrible way that requires
// partially matching and then backtracking through a bunch of cases due to a
// mismatched suffix.

struct Pathological {
  static TextSpan match(TextNodeContext& ctx, TextSpan body) {
    return pattern::match(ctx, body);
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

//----------------------------------------

void test_pathological() {
  printf("test_pathological()\n");
  reset_everything();

  TextNodeContext ctx;

  // Matching this pattern should produce 7 live nodes and 137250 dead nodes.
  auto text = utils::to_span("[[[[[[a]]]]]]");
  auto tail = Pathological::match(ctx, text);
  matcheroni_assert(tail.is_valid() && "pathological tree invalid");

  // Tree should be
  // {[[[[[[a]]]]]]       } *none
  // {[[[[[a]]]]]         }  |-none
  // {[[[[a]]]]           }  | |-none
  // {[[[a]]]             }  | | |-none
  // {[[a]]               }  | | | |-none
  // {[a]                 }  | | | | |-none
  // {a                   }  | | | | | |-atom

  utils::print_summary(text, tail, ctx, 50);
  check_hash(ctx, 0x07a37a832d506209);

  matcheroni_assert(utils::InstanceCounter<TestNode>::live == 7);
  matcheroni_assert(utils::InstanceCounter<TestNode>::dead == 137250);
  matcheroni_assert(LinearAlloc::inst().max_size == sizeof(TestNode) * 7);
  matcheroni_assert(LinearAlloc::inst().current_size == sizeof(TestNode) * 7);

  printf("test_pathological() end\n\n");
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Parseroni tests\n");

  printf("//----------------------------------------\n");
  test_basic();
  printf("//----------------------------------------\n");
  test_rewind();
  printf("//----------------------------------------\n");
  test_begin_end();
  printf("//----------------------------------------\n");
  test_pathological();
  printf("//----------------------------------------\n");

  printf("All tests pass!\n");
  return 0;
}

//------------------------------------------------------------------------------
