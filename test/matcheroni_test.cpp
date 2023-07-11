#include "matcheroni/Matcheroni.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

static int fail_count = 0;

#define CHECK(A, ...)                                                         \
  if (!(A)) {                                                                 \
    fail_count++;                                                             \
    fprintf(stderr, "\n");                                                    \
    fprintf(stderr, "CHECK(" #A ") fail: @ %s/%s:%d", __FILE__, __FUNCTION__, \
            __LINE__);                                                        \
    fprintf(stderr, "\n  " __VA_ARGS__);                                      \
    fprintf(stderr, "\n");                                                    \
  }

using namespace matcheroni;

using cspan = Span<const char>;

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

void test_span() {
  auto span1 = to_span("Hello World");
  auto span2 = span1.advance(strlen("Hello"));
  CHECK((span1 - span2) == "Hello");

  auto span3 = to_span("CDE");

  CHECK(strcmp_span(span3, "CD") > 0);
  CHECK(strcmp_span(span3, "BC") > 0);
  CHECK(strcmp_span(span3, "DE") < 0);

  CHECK(strcmp_span(span3, "CDE") == 0);
  CHECK(strcmp_span(span3, "BCD") > 0);
  CHECK(strcmp_span(span3, "DEF") < 0);

  CHECK(strcmp_span(span3, "CDEF") < 0);
  CHECK(strcmp_span(span3, "BCDE") > 0);
  CHECK(strcmp_span(span3, "DEFG") < 0);
}

//------------------------------------------------------------------------------

void test_atom() {
  cspan text;
  cspan tail;

  // Single atoms should match a single character.
  text = to_span("abc");
  tail = Atom<'a'>::match(nullptr, text);
  CHECK(tail.valid() && tail == "bc");

  // Providing multiple options in an atom matcher should work.
  text = to_span("abc");
  tail = Atom<'b', 'a'>::match(nullptr, text);
  CHECK(tail.valid() && tail == "bc");

  // Failed atom matches should leave the fail cursor at BOL
  text = to_span("abc");
  tail = Atom<'b'>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "abc");

  // AnyAtom should match... any atom
  text = to_span("abc");
  tail = AnyAtom::match(nullptr, text);
  CHECK(tail.valid() && tail == "bc");

  text = to_span("zyx");
  tail = AnyAtom::match(nullptr, text);
  CHECK(tail.valid() && tail == "yx");

  // AnyAtom should not match at EOL
  text = to_span("");
  tail = AnyAtom::match(nullptr, text);
  CHECK(!tail.valid() && tail == "");
}

//------------------------------------------------------------------------------

void test_range() {
  cspan text;
  cspan tail;

  text = to_span("");
  tail = Range<'a', 'z'>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "");

  text = to_span("qr");
  tail = Range<'a', 'z'>::match(nullptr, text);
  CHECK(tail.valid() && tail == "r");

  text = to_span("01");
  tail = Range<'a', 'z'>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "01");
}

//------------------------------------------------------------------------------

void test_lit() {
  cspan text;
  cspan tail;

  text = to_span("");
  tail = Lit<"foo">::match(nullptr, text);
  CHECK(!tail.valid() && tail == "");

  text = to_span("foo");
  tail = Lit<"foo">::match(nullptr, text);
  CHECK(tail.valid() && tail == "");

  text = to_span("foo bar baz");
  tail = Lit<"foo">::match(nullptr, text);
  CHECK(tail.valid() && tail == " bar baz");

  text = to_span("foo bar baz");
  tail = Lit<"bar">::match(nullptr, text);
  CHECK(!tail.valid() && tail == "foo bar baz");

  // Failing lit match should report fail loc at first non-matching char
  text = to_span("abcdefgh");
  tail = Lit<"abcdex">::match(nullptr, text);
  CHECK(!tail.valid() && tail == "fgh");
}

//------------------------------------------------------------------------------

void test_seq() {
  cspan text;
  cspan tail;

  text = to_span("abc");
  tail = Seq<Atom<'a'>, Atom<'b'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "c");

  // A failing seq<> should leave the cursor at the end of the partial match.
  text = to_span("acd");
  tail = Seq<Atom<'a'>, Atom<'b'>>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "cd");
}

//------------------------------------------------------------------------------

void test_oneof() {
  cspan text;
  cspan tail;

  // Order of the oneof<> items if they do _not_ share a prefix should _not_
  // matter
  text = to_span("foo bar baz");
  tail = Oneof<Lit<"foo">, Lit<"bar">>::match(nullptr, text);
  CHECK(tail.valid() && tail == " bar baz");

  text = to_span("foo bar baz");
  tail = Oneof<Lit<"bar">, Lit<"foo">>::match(nullptr, text);
  CHECK(tail.valid() && tail == " bar baz");

  // Order of the oneof<> items if they _do_ share a prefix _should_ matter
  text = to_span("abcdefgh");
  tail = Oneof<Lit<"abc">, Lit<"abcdef">>::match(nullptr, text);
  CHECK(tail.valid() && tail == "defgh");

  tail = Oneof<Lit<"abcdef">, Lit<"abc">>::match(nullptr, text);
  CHECK(tail.valid() && tail == "gh");

  // Failing oneof<> should leave cursor at the end of the largest partial
  // sub-match.
  text = to_span("abcd0");
  tail = Oneof<Lit<"abcdefgh">, Lit<"abcde">, Lit<"xyz">>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "0");
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Matcheroni tests\n");

  test_span();
  test_atom();
  test_range();
  test_lit();
  test_seq();
  test_oneof();

  if (!fail_count) {
    printf("All tests pass!\n");
  } else {
    printf("Failed %d tests!\n", fail_count);
  }

  return 0;
}
