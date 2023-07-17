#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <string>

using namespace matcheroni;

//------------------------------------------------------------------------------

static int fail_count = 0;

#define TEST(A, ...)                                                 \
  if (!(A)) {                                                        \
    fail_count++;                                                    \
    printf("\n");                                                    \
    printf("TEST(" #A ") fail: @ %s/%s:%d", __FILE__, __FUNCTION__,  \
           __LINE__);                                                \
    printf("\n  " __VA_ARGS__);                                      \
    printf("\n");                                                    \
  }

//------------------------------------------------------------------------------

void test_span() {
  auto span1 = to_span("Hello World");
  auto span2 = span1.advance(strlen("Hello"));
  TEST((span1 - span2) == "Hello");

  auto span3 = to_span("CDE");

  TEST(strcmp_span(span3, "CD") > 0);
  TEST(strcmp_span(span3, "BC") > 0);
  TEST(strcmp_span(span3, "DE") < 0);

  TEST(strcmp_span(span3, "CDE") == 0);
  TEST(strcmp_span(span3, "BCD") > 0);
  TEST(strcmp_span(span3, "DEF") < 0);

  TEST(strcmp_span(span3, "CDEF") < 0);
  TEST(strcmp_span(span3, "BCDE") > 0);
  TEST(strcmp_span(span3, "DEFG") < 0);
}

//------------------------------------------------------------------------------

void test_atom() {
  TextSpan text;
  TextSpan tail;

  // Single atoms should match a single character.
  text = to_span("abc");
  tail = Atom<'a'>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bc");

  // Providing multiple options in an atom matcher should work.
  text = to_span("abc");
  tail = Atom<'b', 'a'>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bc");

  // Failed atom matches should leave the fail cursor at BOL
  text = to_span("abc");
  tail = Atom<'b'>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "abc");

  // AnyAtom should match... any atom
  text = to_span("abc");
  tail = AnyAtom::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bc");

  text = to_span("zyx");
  tail = AnyAtom::match(nullptr, text);
  TEST(tail.is_valid() && tail == "yx");

  // AnyAtom should not match at EOL
  text = to_span("");
  tail = AnyAtom::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "");
}

//------------------------------------------------------------------------------

void test_notatom() {
  TextSpan text;
  TextSpan tail;

  text = to_span("");
  tail = NotAtom<'a'>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "");

  text = to_span("abc");
  tail = NotAtom<'a'>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "abc");

  text = to_span("abc");
  tail = NotAtom<'z'>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bc");

  text = to_span("abc");
  tail = NotAtom<'b', 'a'>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "abc");

  text = to_span("abc");
  tail = NotAtom<'z', 'y'>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bc");
};

//------------------------------------------------------------------------------

void test_range() {
  TextSpan text;
  TextSpan tail;

  text = to_span("");
  tail = Range<'a', 'z'>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "");

  text = to_span("qr");
  tail = Range<'a', 'z'>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "r");

  text = to_span("01");
  tail = Range<'a', 'z'>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "01");

  text = to_span("ab");
  tail = NotRange<'a', 'z'>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "ab");

  text = to_span("ab");
  tail = NotRange<'m', 'z'>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "b");
}

//------------------------------------------------------------------------------

void test_lit() {
  TextSpan text;
  TextSpan tail;

  text = to_span("");
  tail = Lit<"foo">::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "");

  text = to_span("foo");
  tail = Lit<"foo">::match(nullptr, text);

  TEST(tail.is_valid() && tail == "");

  text = to_span("foo bar baz");
  tail = Lit<"foo">::match(nullptr, text);
  TEST(tail.is_valid() && tail == " bar baz");

  text = to_span("foo bar baz");
  tail = Lit<"bar">::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "foo bar baz");

  // Failing lit match should report fail loc at first non-matching char
  text = to_span("abcdefgh");
  tail = Lit<"abcdex">::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "fgh");
}

//------------------------------------------------------------------------------

void test_seq() {
  TextSpan text;
  TextSpan tail;

  text = to_span("abc");
  tail = Seq<Atom<'a'>, Atom<'b'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "c");

  // A failing seq<> should leave the cursor at the end of the partial match.
  text = to_span("acd");
  tail = Seq<Atom<'a'>, Atom<'b'>>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "cd");
}

//------------------------------------------------------------------------------

void test_oneof() {
  TextSpan text;
  TextSpan tail;

  // Order of the oneof<> items if they do _not_ share a prefix should _not_
  // matter
  text = to_span("foo bar baz");
  tail = Oneof<Lit<"foo">, Lit<"bar">>::match(nullptr, text);
  TEST(tail.is_valid() && tail == " bar baz");

  text = to_span("foo bar baz");
  tail = Oneof<Lit<"bar">, Lit<"foo">>::match(nullptr, text);
  TEST(tail.is_valid() && tail == " bar baz");

  // Order of the oneof<> items if they _do_ share a prefix _should_ matter
  text = to_span("abcdefgh");
  tail = Oneof<Lit<"abc">, Lit<"abcdef">>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "defgh");

  tail = Oneof<Lit<"abcdef">, Lit<"abc">>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "gh");

  // Failing oneof<> should leave cursor at the end of the largest partial
  // sub-match.
  text = to_span("abcd0");
  tail = Oneof<Lit<"abcdefgh">, Lit<"abcde">, Lit<"xyz">>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "0");
}

//------------------------------------------------------------------------------

void test_opt() {
  TextSpan text;
  TextSpan tail;

  text = to_span("abcd");
  tail = Opt<Atom<'a'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bcd");

  text = to_span("abcd");
  tail = Opt<Atom<'b'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "abcd");
}

//------------------------------------------------------------------------------

void test_any() {
  TextSpan text;
  TextSpan tail;

  text = to_span("");
  tail = Any<Atom<'a'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "");

  text = to_span("aaaabbbb");
  tail = Any<Atom<'a'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bbbb");

  text = to_span("aaaabbbb");
  tail = Any<Atom<'b'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "aaaabbbb");
}

//------------------------------------------------------------------------------

void test_some() {
  TextSpan text;
  TextSpan tail;

  text = to_span("");
  tail = Some<Atom<'a'>>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "");

  text = to_span("aaaabbbb");
  tail = Some<Atom<'a'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bbbb");

  text = to_span("aaaabbbb");
  tail = Some<Atom<'b'>>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "aaaabbbb");
}

//------------------------------------------------------------------------------

void test_and() {
  TextSpan text;
  TextSpan tail;

  text = to_span("");
  tail = And<Atom<'a'>>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "");

  text = to_span("aaaabbbb");
  tail = And<Atom<'a'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "aaaabbbb");

  text = to_span("aaaabbbb");
  tail = And<Atom<'b'>>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "aaaabbbb");
}

//------------------------------------------------------------------------------

void test_not() {
  TextSpan text;
  TextSpan tail;

  text = to_span("");
  tail = Not<Atom<'a'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "");

  text = to_span("aaaabbbb");
  tail = Not<Atom<'a'>>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "aaaabbbb");

  text = to_span("aaaabbbb");
  tail = Not<Atom<'b'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "aaaabbbb");
}

//------------------------------------------------------------------------------

void test_rep() {
  TextSpan text;
  TextSpan tail;

  text = to_span("");
  tail = Rep<3, Atom<'a'>>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "");

  text = to_span("aabbbb");
  tail = Rep<3, Atom<'a'>>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "bbbb");

  text = to_span("aaabbbb");
  tail = Rep<3, Atom<'a'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bbbb");

  text = to_span("aaaabbbb");
  tail = Rep<3, Atom<'a'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "abbbb");
}

//------------------------------------------------------------------------------

void test_reprange() {
  TextSpan text;
  TextSpan tail;

  text = to_span("");
  tail = RepRange<2, 3, Atom<'a'>>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "");

  text = to_span("abbbb");
  tail = RepRange<2, 3, Atom<'a'>>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "bbbb");

  text = to_span("aabbbb");
  tail = RepRange<2, 3, Atom<'a'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bbbb");

  text = to_span("aaabbbb");
  tail = RepRange<2, 3, Atom<'a'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bbbb");

  text = to_span("aaaabbbb");
  tail = RepRange<2, 3, Atom<'a'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "abbbb");
}

//------------------------------------------------------------------------------

void test_until() {
  TextSpan text;
  TextSpan tail;

  text = to_span("");
  tail = Until<Atom<'b'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "");

  text = to_span("aaaa");
  tail = Until<Atom<'b'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "");

  text = to_span("aaaabbbb");
  tail = Until<Atom<'b'>>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bbbb");
}


//------------------------------------------------------------------------------

TextSpan test_matcher(void* ctx, TextSpan s) {
  matcheroni_assert(s.is_valid());
  if (s.is_empty()) return s.fail();
  return s.a[0] == 'a' ? s.advance(1) : s.fail();
}

void test_ref() {
  TextSpan text;
  TextSpan tail;

  text = to_span("");
  tail = Ref<test_matcher>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "");

  text = to_span("abc");
  tail = Ref<test_matcher>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bc");

  text = to_span("xyz");
  tail = Ref<test_matcher>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "xyz");
}

//------------------------------------------------------------------------------

void test_backref() {
  TextSpan text;
  TextSpan tail;

  using pattern1 =
      Seq<StoreBackref<"backref", char, Rep<4, Range<'a', 'z'>>>,
          Atom<'-'>,
          MatchBackref<"backref", char, Rep<4, Range<'a', 'z'>>>>;

  text = to_span("abcd-abcd!");
  tail = pattern1::match(nullptr, text);
  TEST(tail.is_valid() && tail == "!");

  text = to_span("zyxw-zyxw!");
  tail = pattern1::match(nullptr, text);
  TEST(tail.is_valid() && tail == "!");

  text = to_span("abcd-abqd!");
  tail = pattern1::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "qd!");

  text = to_span("abcd-abc");
  tail = pattern1::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "");

  text = to_span("ab01-ab01!");
  tail = pattern1::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "01-ab01!");
}

//------------------------------------------------------------------------------

void test_delimited_block() {
  TextSpan text;
  TextSpan tail;

  using pattern = DelimitedBlock<Atom<'{'>, Atom<'a'>, Atom<'}'>>;

  text = to_span("{}bbbb");
  tail = pattern::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bbbb");

  text = to_span("{a}bbbb");
  tail = pattern::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bbbb");

  text = to_span("{aa}bbbb");
  tail = pattern::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bbbb");

  text = to_span("{bb}aaaa");
  tail = pattern::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "bb}aaaa");

  text = to_span("{aabb}bbbb");
  tail = pattern::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "bb}bbbb");

  text = to_span("{aaaa");
  tail = pattern::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "");
}

//------------------------------------------------------------------------------

void test_delimited_list() {
  TextSpan text;
  TextSpan tail;

  using pattern = DelimitedList<Atom<'{'>, Atom<'a'>, Atom<','>, Atom<'}'>>;

  // Zero items
  text = to_span("{}bbbb");
  tail = pattern::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bbbb");

  // One item
  text = to_span("{a}bbbb");
  tail = pattern::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bbbb");

  // Two items
  text = to_span("{a,a}bbbb");
  tail = pattern::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bbbb");

  // Two items + trailing comma
  text = to_span("{a,a,}bbbb");
  tail = pattern::match(nullptr, text);
  TEST(tail.is_valid() && tail == "bbbb");

  // Ldelim missing
  text = to_span("a,a}bbbb");
  tail = pattern::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "a,a}bbbb");

  // Item missing
  text = to_span("{,a}bbbb");
  tail = pattern::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == ",a}bbbb");

  // Separator missing
  text = to_span("{aa}bbbb");
  tail = pattern::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "a}bbbb");

  // Rdelim missing
  text = to_span("{a,abbbb");
  tail = pattern::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "bbbb");

  // Wrong separator
  text = to_span("{a;a}bbbb");
  tail = pattern::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == ";a}bbbb");

  // Wrong item
  text = to_span("{a,b}bbbb");
  tail = pattern::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "b}bbbb");
}

//------------------------------------------------------------------------------

void test_eol() {
  TextSpan text;
  TextSpan tail;

  text = to_span("aaaa");
  tail = Seq<Some<Atom<'a'>>, EOL>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "");

  text = to_span("aaaabbbb");
  tail = Seq<Some<Atom<'a'>>, EOL>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "bbbb");

  text = to_span("aaaa\nbbbb");
  tail = Seq<Some<Atom<'a'>>, EOL>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "\nbbbb");
}

//------------------------------------------------------------------------------

void test_charset() {
  TextSpan text;
  TextSpan tail;

  text = to_span("dcbaxxxx");
  tail = Some<Charset<"abcd">>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "xxxx");

  text = to_span("xxxxabcd");
  tail = Some<Charset<"abcd">>::match(nullptr, text);
  TEST(!tail.is_valid() && std::string(tail.b) == "xxxxabcd");

  text = to_span("ddccxxxxbbaa");
  tail = Some<Charset<"abcd">>::match(nullptr, text);
  TEST(tail.is_valid() && tail == "xxxxbbaa");
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
  test_until();
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