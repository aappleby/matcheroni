#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

using namespace matcheroni;
using cspan = Span<const char>;

struct TestNode : public NodeBase {
  using NodeBase::NodeBase;

  void dump_tree(std::string& out) {
    if (strcmp(match_name, "atom") == 0) {
      for (auto c = span.a; c < span.b; c++) out.push_back(*c);
    }
    else if (strcmp(match_name, "list") == 0) {
      out.push_back('(');
      for (auto c = (TestNode*)child_head; c; c = (TestNode*)c->node_next) {
        c->dump_tree(out);
        if (c->node_next) out.push_back(',');
      }
      out.push_back(')');
    }
    else {
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

cspan to_span(const char* text) { return cspan(text, text + strlen(text)); }

cspan to_span(const std::string& s) {
  return cspan(s.data(), s.data() + s.size());
}

std::string to_string(cspan s) {
  return s.a ? std::string(s.a, s.b) : std::string(s.b);
}

bool operator==(cspan a, const std::string& b) { return to_string(a) == b; }
bool operator==(const std::string& a, cspan b) { return a == to_string(b); }
bool operator!=(cspan a, const std::string& b) { return to_string(a) != b; }
bool operator!=(const std::string& a, cspan b) { return a != to_string(b); }

//------------------------------------------------------------------------------

cspan match_element(void* ctx, cspan s);
using element = Ref<match_element>;

using ws   = Any<Atom<' ', '\n', '\r', '\t'>>;
using atom = Some<NotAtom<'(',')',' ', '\n', '\r', '\t', ','>>;
using list =
Seq<
  Atom<'('>,
  ws,
  Opt<element>,
  Any<Seq<ws, Atom<','>, ws, element>>,
  ws,
  Atom<')'>
>;

cspan match_element(void* ctx, cspan s) {
  using pattern =
  Oneof<
    CaptureNamed<"atom", atom, TestNode>,
    CaptureNamed<"list", list, TestNode>
  >;

  return pattern::match(ctx, s);
}

//------------------------------------------------------------------------------

void test_basic() {
  Parser p;
  cspan span;
  cspan tail;

  std::string text = "(abcd,efgh,(ab),(a,(bc,de)),ghijk)";

  p.reset();
  span = to_span(text);
  tail = element::match(&p, span);
  CHECK(tail.valid() && tail == "");
  //for (auto c = p.top_head; c; c = c->node_next) print_tree(c);
  //printf("tree hash 0x%016lX\n", p.top_head->hash());

  std::string dump;
  ((TestNode*)p.top_head)->dump_tree(dump);
  CHECK(text == dump, "We should be able to parse and print a list");
  //printf("%s\n", dump.c_str());
  //printf("%s\n", text.c_str());

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
