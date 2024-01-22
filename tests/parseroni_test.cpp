#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

#include <stdio.h>
#include <string.h>
#include <string>

using namespace matcheroni;
using namespace parseroni;

//------------------------------------------------------------------------------

struct TestNode : public NodeBase<TestNode, char>, public utils::InstanceCounter<TestNode> {
  TextSpan as_text_span() const { return span; }
};

struct TestContext : public NodeContext<TestNode> {
  static int atom_cmp(char a, int b) { return (unsigned char)a - b; }
};

//------------------------------------------------------------------------------

void sexp_to_string(TestNode* n, std::string& out) {
  if (strcmp(n->match_tag, "atom") == 0) {
    for (auto c = n->span.begin; c < n->span.end; c++) out.push_back(*c);
  } else if (strcmp(n->match_tag, "list") == 0) {
    out.push_back('(');
    for (auto c = n->child_head; c; c = c->node_next) {
      sexp_to_string((TestNode*)c, out);
      if (c->node_next) out.push_back(',');
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
  //printf("Expected hash 0x%016lx\n", hash_a);
  //printf("Actual hash   0x%016lx\n", hash_b);
  matcheroni_assert(hash_a == hash_b && "bad hash");
}

//----------------------------------------

void reset_everything() {
  TestNode::reset_count();
  matcheroni_assert(TestNode::live == 0);
  matcheroni_assert(TestNode::dead == 0);
}

//------------------------------------------------------------------------------
// A mini s-expression parser in ~10 lines of code. :D

struct SExpression {
  static TextSpan match(TestContext& ctx, TextSpan body) {
    return Oneof<
      Capture<"atom", atom, TestNode>,
      Capture<"list", list, TestNode>
    >::match(ctx, body);
  }

  using space = Some<Atoms<' ', '\n', '\r', '\t'>>;
  using atom  = Some<Ranges<'0','9','a','z','A','Z', '_', '_'>>;
  using car   = Ref<match>;
  using cdr   = Some<Seq<Atom<','>, Any<space>, Ref<match>>>;
  using list  = Seq<Atom<'('>, Opt<space>, Opt<car>, Opt<space>, Opt<cdr>, Opt<space>, Atom<')'>>;
};

//----------------------------------------

void test_basic() {
  //printf("test_basic()\n");
  reset_everything();

  {
    // Check than we can round-trip a s-expression

    std::string expression = "(abcd,efgh,(ab),(a,(bc,de)),ghijk)";

    TestContext ctx;
    ctx.reset();
    auto text = utils::to_span(expression);
    auto tail = SExpression::match(ctx, text);
    matcheroni_assert(tail.is_valid() && tail.is_empty());

    //utils::print_summary(ctx, text, tail, 50);

    //printf("Round-trip s-expression:\n");
    std::string new_text;
    sexp_to_string((TestNode*)ctx.top_head, new_text);
    //printf("Old : %s\n", text.begin);
    //printf("New : %s\n", new_text.c_str());
    matcheroni_assert(expression == new_text && "Mismatch!");
    //printf("\n");

    //utils::print_summary(ctx, text, tail, 50);

    check_hash(ctx, 0x7073c4e1b84277f0);

    matcheroni_assert(TestNode::live == 11);
    matcheroni_assert(TestNode::dead == 0);
  }

  TestContext ctx;
  TextSpan span;
  TextSpan tail;

  ctx.reset();
  span = utils::to_span("((((a))))");
  tail = SExpression::match(ctx, span);
  matcheroni_assert(tail.is_valid() && tail.is_empty());

  ctx.reset();
  span = utils::to_span("(((())))");
  tail = SExpression::match(ctx, span);
  matcheroni_assert(tail.is_valid() && tail.is_empty());

  ctx.reset();
  span = utils::to_span("(((()))(");
  tail = SExpression::match(ctx, span);
  matcheroni_assert(!tail.is_valid() && std::string(tail.end) == "(");

  //printf("test_basic() end\n\n");
}

//------------------------------------------------------------------------------

void test_rewind() {
  //printf("test_rewind()\n");
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

  TestContext ctx;

  auto text = utils::to_span("abcdef");
  auto tail = pattern::match(ctx, text);

  //utils::print_summary(ctx, text, tail, 50);
  check_hash(ctx, 0x2850a87bce45242a);

  matcheroni_assert(TestNode::live == 1);
  matcheroni_assert(TestNode::dead == 5);

  //printf("test_rewind() end\n\n");
}

//------------------------------------------------------------------------------

struct BeginEndTest {

  static TextSpan match(TestContext& ctx, TextSpan body) {
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

  using space = Some<Atoms<' ', '\n', '\r', '\t'>>;
  using atom  = Some<Ranges<'0','9','a','f','A','F'>>;
  using car   = Opt<Ref<match>>;
  using cdr   = Any<Seq<Atom<','>, Any<space>, Ref<match>>>;
  using list  = Seq<Atom<'['>, Any<space>, car, Any<space>, cdr, Any<space>, Atom<']'>>;
};

//------------------------------------------------------------------------------

void test_begin_end() {
  //printf("test_begin_end()\n");
  reset_everything();

  TestContext ctx;

  auto text = utils::to_span("[ [abc,ab?,cdb+] , [a,b,c*,d,e,f] ]");
  auto tail = BeginEndTest::match(ctx, text);

  //utils::print_summary(ctx, text, tail, 50);
  check_hash(ctx, 0x8c3ca2b021e9a9b3);

  matcheroni_assert(TestNode::live == 15);
  matcheroni_assert(TestNode::dead == 0);

  //printf("test_begin_end() end\n\n");
}

//------------------------------------------------------------------------------
// This matcher matches nested square brackets with a single letter in the
// middle, but it does so in a pathologically-horrible way that requires
// partially matching and then backtracking through a bunch of cases due to a
// mismatched suffix.

struct Pathological {
  static TextSpan match(TestContext& ctx, TextSpan body) {
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
  //printf("test_pathological()\n");
  reset_everything();

  TestContext ctx;

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

  //utils::print_summary(ctx, text, tail, 50);
  check_hash(ctx, 0x07a37a832d506209);

  matcheroni_assert(TestNode::live == 7);
  matcheroni_assert(TestNode::dead == 137250);

  //printf("test_pathological() end\n\n");
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  test_basic();
  test_rewind();
  test_begin_end();
  test_pathological();
  return 0;
}

//------------------------------------------------------------------------------
