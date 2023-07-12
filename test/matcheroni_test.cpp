#include "matcheroni/Matcheroni.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

using namespace matcheroni;
using cspan = Span<const char>;

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

void test_notatom() {
  cspan text;
  cspan tail;

  text = to_span("");
  tail = NotAtom<'a'>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "");

  text = to_span("abc");
  tail = NotAtom<'a'>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "abc");

  text = to_span("abc");
  tail = NotAtom<'z'>::match(nullptr, text);
  CHECK(tail.valid() && tail == "bc");

  text = to_span("abc");
  tail = NotAtom<'b', 'a'>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "abc");

  text = to_span("abc");
  tail = NotAtom<'z', 'y'>::match(nullptr, text);
  CHECK(tail.valid() && tail == "bc");
};

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

  text = to_span("ab");
  tail = NotRange<'a', 'z'>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "ab");

  text = to_span("ab");
  tail = NotRange<'m', 'z'>::match(nullptr, text);
  CHECK(tail.valid() && tail == "b");
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

void test_opt() {
  cspan text;
  cspan tail;

  text = to_span("abcd");
  tail = Opt<Atom<'a'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "bcd");

  text = to_span("abcd");
  tail = Opt<Atom<'b'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "abcd");
}

//------------------------------------------------------------------------------

void test_any() {
  cspan text;
  cspan tail;

  text = to_span("");
  tail = Any<Atom<'a'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "");

  text = to_span("aaaabbbb");
  tail = Any<Atom<'a'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "bbbb");

  text = to_span("aaaabbbb");
  tail = Any<Atom<'b'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "aaaabbbb");
}

//------------------------------------------------------------------------------

void test_some() {
  cspan text;
  cspan tail;

  text = to_span("");
  tail = Some<Atom<'a'>>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "");

  text = to_span("aaaabbbb");
  tail = Some<Atom<'a'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "bbbb");

  text = to_span("aaaabbbb");
  tail = Some<Atom<'b'>>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "aaaabbbb");
}

//------------------------------------------------------------------------------

void test_and() {
  cspan text;
  cspan tail;

  text = to_span("");
  tail = And<Atom<'a'>>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "");

  text = to_span("aaaabbbb");
  tail = And<Atom<'a'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "aaaabbbb");

  text = to_span("aaaabbbb");
  tail = And<Atom<'b'>>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "aaaabbbb");
}

//------------------------------------------------------------------------------

void test_not() {
  cspan text;
  cspan tail;

  text = to_span("");
  tail = Not<Atom<'a'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "");

  text = to_span("aaaabbbb");
  tail = Not<Atom<'a'>>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "aaaabbbb");

  text = to_span("aaaabbbb");
  tail = Not<Atom<'b'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "aaaabbbb");
}

//------------------------------------------------------------------------------

void test_rep() {
  cspan text;
  cspan tail;

  text = to_span("");
  tail = Rep<3, Atom<'a'>>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "");

  text = to_span("aabbbb");
  tail = Rep<3, Atom<'a'>>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "bbbb");

  text = to_span("aaabbbb");
  tail = Rep<3, Atom<'a'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "bbbb");

  text = to_span("aaaabbbb");
  tail = Rep<3, Atom<'a'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "abbbb");
}

//------------------------------------------------------------------------------

void test_reprange() {
  cspan text;
  cspan tail;

  text = to_span("");
  tail = RepRange<2, 3, Atom<'a'>>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "");

  text = to_span("abbbb");
  tail = RepRange<2, 3, Atom<'a'>>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "bbbb");

  text = to_span("aabbbb");
  tail = RepRange<2, 3, Atom<'a'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "bbbb");

  text = to_span("aaabbbb");
  tail = RepRange<2, 3, Atom<'a'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "bbbb");

  text = to_span("aaaabbbb");
  tail = RepRange<2, 3, Atom<'a'>>::match(nullptr, text);
  CHECK(tail.valid() && tail == "abbbb");
}

//------------------------------------------------------------------------------

cspan test_matcher(void* ctx, cspan s) {
  assert(s.valid());
  if (s.empty()) return s.fail();
  return s.a[0] == 'a' ? s.advance(1) : s.fail();
}

void test_ref() {
  cspan text;
  cspan tail;

  text = to_span("");
  tail = Ref<test_matcher>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "");

  text = to_span("abc");
  tail = Ref<test_matcher>::match(nullptr, text);
  CHECK(tail.valid() && tail == "bc");

  text = to_span("xyz");
  tail = Ref<test_matcher>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "xyz");
}

//------------------------------------------------------------------------------

void test_backref() {
  cspan text;
  cspan tail;

  using pattern1 =
      Seq<StoreBackref<const char, Rep<4, Range<'a', 'z'>>>, Atom<'-'>,
          MatchBackref<const char, Rep<4, Range<'a', 'z'>>>>;

  text = to_span("abcd-abcd!");
  tail = pattern1::match(nullptr, text);
  CHECK(tail.valid() && tail == "!");

  text = to_span("zyxw-zyxw!");
  tail = pattern1::match(nullptr, text);
  CHECK(tail.valid() && tail == "!");

  text = to_span("abcd-abqd!");
  tail = pattern1::match(nullptr, text);
  CHECK(!tail.valid() && tail == "qd!");

  text = to_span("abcd-abc");
  tail = pattern1::match(nullptr, text);
  CHECK(!tail.valid() && tail == "");

  text = to_span("ab01-ab01!");
  tail = pattern1::match(nullptr, text);
  CHECK(!tail.valid() && tail == "01-ab01!");
}

//------------------------------------------------------------------------------

void test_delimited_block() {
  cspan text;
  cspan tail;

  using pattern = DelimitedBlock<Atom<'{'>, Atom<'a'>, Atom<'}'>>;

  text = to_span("{}bbbb");
  tail = pattern::match(nullptr, text);
  CHECK(tail.valid() && tail == "bbbb");

  text = to_span("{a}bbbb");
  tail = pattern::match(nullptr, text);
  CHECK(tail.valid() && tail == "bbbb");

  text = to_span("{aa}bbbb");
  tail = pattern::match(nullptr, text);
  CHECK(tail.valid() && tail == "bbbb");

  text = to_span("{bb}aaaa");
  tail = pattern::match(nullptr, text);
  CHECK(!tail.valid() && tail == "bb}aaaa");

  text = to_span("{aabb}bbbb");
  tail = pattern::match(nullptr, text);
  CHECK(!tail.valid() && tail == "bb}bbbb");

  text = to_span("{aaaa");
  tail = pattern::match(nullptr, text);
  CHECK(!tail.valid() && tail == "");
}

//------------------------------------------------------------------------------

void test_delimited_list() {
  cspan text;
  cspan tail;

  using pattern = DelimitedList<Atom<'{'>, Atom<'a'>, Atom<','>, Atom<'}'>>;

  // Zero items
  text = to_span("{}bbbb");
  tail = pattern::match(nullptr, text);
  CHECK(tail.valid() && tail == "bbbb");

  // One item
  text = to_span("{a}bbbb");
  tail = pattern::match(nullptr, text);
  CHECK(tail.valid() && tail == "bbbb");

  // Two items
  text = to_span("{a,a}bbbb");
  tail = pattern::match(nullptr, text);
  CHECK(tail.valid() && tail == "bbbb");

  // Two items + trailing comma
  text = to_span("{a,a,}bbbb");
  tail = pattern::match(nullptr, text);
  CHECK(tail.valid() && tail == "bbbb");

  // Ldelim missing
  text = to_span("a,a}bbbb");
  tail = pattern::match(nullptr, text);
  CHECK(!tail.valid() && tail == "a,a}bbbb");

  // Item missing
  text = to_span("{,a}bbbb");
  tail = pattern::match(nullptr, text);
  CHECK(!tail.valid() && tail == ",a}bbbb");

  // Separator missing
  text = to_span("{aa}bbbb");
  tail = pattern::match(nullptr, text);
  CHECK(!tail.valid() && tail == "a}bbbb");

  // Rdelim missing
  text = to_span("{a,abbbb");
  tail = pattern::match(nullptr, text);
  CHECK(!tail.valid() && tail == "bbbb");

  // Wrong separator
  text = to_span("{a;a}bbbb");
  tail = pattern::match(nullptr, text);
  CHECK(!tail.valid() && tail == ";a}bbbb");

  // Wrong item
  text = to_span("{a,b}bbbb");
  tail = pattern::match(nullptr, text);
  CHECK(!tail.valid() && tail == "b}bbbb");
}

//------------------------------------------------------------------------------

void test_eol() {
  cspan text;
  cspan tail;

  text = to_span("aaaa");
  tail = Seq<Some<Atom<'a'>>, EOL>::match(nullptr, text);
  CHECK(tail.valid() & tail == "");

  text = to_span("aaaabbbb");
  tail = Seq<Some<Atom<'a'>>, EOL>::match(nullptr, text);
  CHECK(!tail.valid() & tail == "bbbb");

  text = to_span("aaaa\nbbbb");
  tail = Seq<Some<Atom<'a'>>, EOL>::match(nullptr, text);
  CHECK(tail.valid() & tail == "\nbbbb");
}

//------------------------------------------------------------------------------

void test_charset() {
  cspan text;
  cspan tail;

  text = to_span("dcbaxxxx");
  tail = Some<Charset<"abcd">>::match(nullptr, text);
  CHECK(tail.valid() && tail == "xxxx");

  text = to_span("xxxxabcd");
  tail = Some<Charset<"abcd">>::match(nullptr, text);
  CHECK(!tail.valid() && tail == "xxxxabcd");

  text = to_span("ddccxxxxbbaa");
  tail = Some<Charset<"abcd">>::match(nullptr, text);
  CHECK(tail.valid() && tail == "xxxxbbaa");
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Matcheroni tests\n");

  test_span();
  test_atom();
  test_notatom();
  test_range();
  test_lit();
  test_seq();
  test_oneof();
  test_opt();
  test_any();
  test_some();
  test_and();
  test_rep();
  test_reprange();
  test_ref();
  test_backref();
  test_delimited_block();
  test_delimited_list();
  test_eol();
  test_charset();

  if (!fail_count) {
    printf("All tests pass!\n");
    return 0;
  } else {
    printf("Failed %d tests!\n", fail_count);
    return -1;
  }
}

//------------------------------------------------------------------------------
