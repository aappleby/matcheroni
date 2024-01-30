#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

#include <stdio.h>
#include <string.h>
#include <string>

#include "dummy.h"

//a();

using namespace matcheroni;

TextMatchContext ctx;

bool operator == (TextSpan const& a, const char* text) {
  if (!a) return false;
  return strcmp_span(a, text) == 0;
}

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
  auto span = utils::to_span("CDE");

  TEST(strcmp_span(span, "CD") > 0);
  TEST(strcmp_span(span, "BC") > 0);
  TEST(strcmp_span(span, "DE") < 0);

  TEST(strcmp_span(span, "CDE") == 0);
  TEST(strcmp_span(span, "BCD") > 0);
  TEST(strcmp_span(span, "DEF") < 0);

  TEST(strcmp_span(span, "CDEF") < 0);
  TEST(strcmp_span(span, "BCDE") > 0);
  TEST(strcmp_span(span, "DEFG") < 0);
}

//------------------------------------------------------------------------------

void test_atom() {
  {
    TextSpan text;
    TextSpan tail;

    // Single atoms should match a single character.
    text = utils::to_span("abc");
    tail = Atom<'a'>::match(ctx, text);
    TEST(tail.is_valid() && tail == "bc");

    // Providing multiple options in an atom matcher should work.
    text = utils::to_span("abc");
    tail = Atoms<'b', 'a'>::match(ctx, text);
    TEST(tail.is_valid() && tail == "bc");

    // Failed atom matches should leave the fail cursor at BOL
    text = utils::to_span("abc");
    tail = Atom<'b'>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "abc");

    // AnyAtom should match... any atom
    text = utils::to_span("abc");
    tail = AnyAtom::match(ctx, text);
    TEST(tail.is_valid() && tail == "bc");

    text = utils::to_span("zyx");
    tail = AnyAtom::match(ctx, text);
    TEST(tail.is_valid() && tail == "yx");

    // AnyAtom should not match at EOL
    text = utils::to_span("");
    tail = AnyAtom::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "");
  }

  {
    TextSpan head, tail;

    tail = utils::to_span("abcde");
    head = Atom<'a'>::take(ctx, tail);
    TEST(head == "a" && tail == "bcde");

    head = Atom<'b'>::take(ctx, tail);
    TEST(head == "b" && tail == "cde");

    head = Atom<'z'>::take(ctx, tail);
    TEST(!head && tail == "cde");

    head = Atoms<'c', 'd'>::take(ctx, tail);
    TEST(head == "c" && tail == "de");

    head = Atoms<'c', 'd'>::take(ctx, tail);
    TEST(head == "d" && tail == "e");

    head = AnyAtom::take(ctx, tail);
    TEST(head == "e" && tail == "");

    head = Atom<'e'>::take(ctx, tail);
    TEST(!head && tail == "");
  }

  /*
  {
    ctx.span = utils::to_span("abcde");
    TextSpan head;

    head = ctx.take<Atom<'a'>>();
    TEST(head == "a" && ctx.span == "bcde");

    head = ctx.take<Atom<'b'>>();
    TEST(head == "b" && ctx.span == "cde");

    head = ctx.take<Atom<'z'>>();
    TEST(!head && ctx.span == "cde");

    head = ctx.take<Atoms<'c', 'd'>>();
    TEST(head == "c" && ctx.span == "de");

    head = ctx.take<Atoms<'c', 'd'>>();
    TEST(head == "d" && ctx.span == "e");

    head = ctx.take<AnyAtom>();
    TEST(head == "e" && ctx.span == "");

    head = ctx.take<Atom<'e'>>();
    TEST(!head && ctx.span == "");
  }
  */
}

//------------------------------------------------------------------------------

void test_notatom() {
  {
    TextSpan text;
    TextSpan tail;

    text = utils::to_span("");
    tail = NotAtom<'a'>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "");

    text = utils::to_span("abc");
    tail = NotAtom<'a'>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "abc");

    text = utils::to_span("abc");
    tail = NotAtom<'z'>::match(ctx, text);
    TEST(tail.is_valid() && tail == "bc");

    text = utils::to_span("abc");
    tail = NotAtoms<'b', 'a'>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "abc");

    text = utils::to_span("abc");
    tail = NotAtoms<'z', 'y'>::match(ctx, text);
    TEST(tail.is_valid() && tail == "bc");
  }

  {
    TextSpan tail;
    TextSpan head;

    tail = utils::to_span("");
    head = NotAtom<'a'>::take(ctx, tail);
    TEST(!head && tail == "");

    tail = utils::to_span("abc");
    head = NotAtom<'a'>::take(ctx, tail);
    TEST(!head && tail == "abc");

    tail = utils::to_span("abc");
    head = NotAtom<'z'>::take(ctx, tail);
    TEST(head == "a" && tail == "bc");

    tail = utils::to_span("abc");
    head = NotAtoms<'b', 'a'>::take(ctx, tail);
    TEST(!head && tail == "abc");

    tail = utils::to_span("abc");
    head = NotAtoms<'z', 'y'>::take(ctx, tail);
    TEST(head == "a" && tail == "bc");
  }
};

//------------------------------------------------------------------------------

void test_range() {
  {
    TextSpan text;
    TextSpan tail;

    text = utils::to_span("");
    tail = Range<'a', 'z'>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "");

    text = utils::to_span("qr");
    tail = Range<'a', 'z'>::match(ctx, text);
    TEST(tail.is_valid() && tail == "r");

    text = utils::to_span("01");
    tail = Range<'a', 'z'>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "01");

    text = utils::to_span("ab");
    tail = NotRange<'a', 'z'>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "ab");

    text = utils::to_span("ab");
    tail = NotRange<'m', 'z'>::match(ctx, text);
    TEST(tail.is_valid() && tail == "b");

    text = utils::to_span("be");
    tail = Ranges<'a','c', 'd', 'f'>::match(ctx, text);
    TEST(tail.is_valid() && tail == "e");

    text = utils::to_span("ez");
    tail = Ranges<'a','c', 'd', 'f'>::match(ctx, text);
    TEST(tail.is_valid() && tail == "z");

    text = utils::to_span("zq");
    tail = Ranges<'a','c', 'd', 'f'>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "zq");

    text = utils::to_span("mn");
    tail = NotRanges<'a','c', 'd','f'>::match(ctx, text);
    TEST(tail.is_valid() && tail == "n");

    text = utils::to_span("be");
    tail = NotRanges<'a','c', 'd','f'>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "be");

    text = utils::to_span("eb");
    tail = NotRanges<'a','c', 'd','f'>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "eb");
  }

  {
    TextSpan head, tail;

    tail = utils::to_span("");
    head = Range<'a', 'z'>::take(ctx, tail);
    TEST(!head && tail == "");

    tail = utils::to_span("qr");
    head = Range<'a', 'z'>::take(ctx, tail);
    TEST(head == "q" && tail == "r");

    tail = utils::to_span("01");
    head = Range<'a', 'z'>::match(ctx, tail);
    TEST(!head && tail == "01");

    tail = utils::to_span("ab");
    head = NotRange<'a', 'z'>::take(ctx, tail);
    TEST(!head && tail == "ab");

    head = NotRange<'m', 'z'>::take(ctx, tail);
    TEST(head == "a" && tail == "b");

    tail = utils::to_span("be");
    head = Ranges<'a','c', 'd', 'f'>::take(ctx, tail);
    TEST(head == "b" && tail == "e");

    tail = utils::to_span("ez");
    head = Ranges<'a','c', 'd', 'f'>::take(ctx, tail);
    TEST(head == "e" && tail == "z");

    tail = utils::to_span("zq");
    head = Ranges<'a','c', 'd', 'f'>::take(ctx, tail);
    TEST(!head && tail == "zq");

    tail = utils::to_span("mn");
    head = NotRanges<'a','c', 'd','f'>::take(ctx, tail);
    TEST(head == "m" && tail == "n");

    tail = utils::to_span("be");
    head = NotRanges<'a','c', 'd','f'>::take(ctx, tail);
    TEST(!head && tail == "be");

    tail = utils::to_span("eb");
    head = NotRanges<'a','c', 'd','f'>::take(ctx, tail);
    TEST(!head && tail == "eb");
  }
}

//------------------------------------------------------------------------------

void test_lit() {
  {
    TextSpan text;
    TextSpan tail;

    text = utils::to_span("");
    tail = Lit<"foo">::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "");

    text = utils::to_span("foo");
    tail = Lit<"foo">::match(ctx, text);

    TEST(tail.is_valid() && tail == "");

    text = utils::to_span("foo bar baz");
    tail = Lit<"foo">::match(ctx, text);
    TEST(tail.is_valid() && tail == " bar baz");

    text = utils::to_span("foo bar baz");
    tail = Lit<"bar">::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "foo bar baz");

    text = utils::to_span("abcdefgh");
    tail = Lit<"abcdex">::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "abcdefgh");
  }

  {
    TextSpan head, tail;

    tail = utils::to_span("");
    head = Lit<"foo">::take(ctx, tail);
    TEST(!head && tail == "");

    tail = utils::to_span("foo");
    head = Lit<"foo">::take(ctx, tail);
    TEST(head == "foo" && tail == "");

    tail = utils::to_span("foo bar baz");
    head = Lit<"foo">::take(ctx, tail);
    TEST(head == "foo" && tail == " bar baz");

    tail = utils::to_span("foo bar baz");
    head = Lit<"bar">::take(ctx, tail);
    TEST(!head && tail == "foo bar baz");

    tail = utils::to_span("abcdefgh");
    head = Lit<"abcdex">::take(ctx, tail);
    TEST(!head && tail == "abcdefgh");
  }
}

//------------------------------------------------------------------------------

void test_seq() {
  {
    TextSpan text;
    TextSpan tail;

    text = utils::to_span("abc");
    tail = Seq<Atom<'a'>, Atom<'b'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "c");

    // A failing seq<> should leave the cursor at the end of the partial match.
    text = utils::to_span("acd");
    tail = Seq<Atom<'a'>, Atom<'b'>>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "cd");
  }

  {
    TextSpan head, tail;

    tail = utils::to_span("abc");
    head = Seq<Atom<'a'>, Atom<'b'>>::take(ctx, tail);
    TEST(head == "ab" && tail == "c");

    // A failing seq<> should leave the cursor at the end of the partial match.
    // FIXME we're not checking fail locations of failed take's
    tail = utils::to_span("acd");
    head = Seq<Atom<'a'>, Atom<'b'>>::take(ctx, tail);
    TEST(!head && tail == "acd");
  }
}

//------------------------------------------------------------------------------

void test_oneof() {
  {
    TextSpan text;
    TextSpan tail;

    // Order of the oneof<> items if they do _not_ share a prefix should _not_
    // matter
    text = utils::to_span("foo bar baz");
    tail = Oneof<Lit<"foo">, Lit<"bar">>::match(ctx, text);
    TEST(tail.is_valid() && tail == " bar baz");

    text = utils::to_span("foo bar baz");
    tail = Oneof<Lit<"bar">, Lit<"foo">>::match(ctx, text);
    TEST(tail.is_valid() && tail == " bar baz");

    // Order of the oneof<> items if they _do_ share a prefix _should_ matter
    text = utils::to_span("abcdefgh");
    tail = Oneof<Lit<"abc">, Lit<"abcdef">>::match(ctx, text);
    TEST(tail.is_valid() && tail == "defgh");

    tail = Oneof<Lit<"abcdef">, Lit<"abc">>::match(ctx, text);
    TEST(tail.is_valid() && tail == "gh");

    text = utils::to_span("abcd0");
    tail = Oneof<Lit<"abcdefgh">, Lit<"abcde">, Lit<"xyz">>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "abcd0");
  }

  {
    TextSpan head, tail;

    // Order of the oneof<> items if they do _not_ share a prefix should _not_
    // matter
    tail = utils::to_span("foo bar baz");
    head = Oneof<Lit<"foo">, Lit<"bar">>::take(ctx, tail);
    TEST(head == "foo" && tail == " bar baz");

    tail = utils::to_span("foo bar baz");
    head = Oneof<Lit<"bar">, Lit<"foo">>::take(ctx, tail);
    TEST(head == "foo" && tail == " bar baz");

    // Order of the oneof<> items if they _do_ share a prefix _should_ matter
    tail = utils::to_span("abcdefgh");
    head = Oneof<Lit<"abc">, Lit<"abcdef">>::take(ctx, tail);
    TEST(head == "abc" && tail == "defgh");

    tail = utils::to_span("abcdefgh");
    head = Oneof<Lit<"abcdef">, Lit<"abc">>::take(ctx, tail);
    TEST(head == "abcdef" && tail == "gh");

    tail = utils::to_span("abcd0");
    head = Oneof<Lit<"abcdefgh">, Lit<"abcde">, Lit<"xyz">>::match(ctx, tail);
    TEST(!head && tail == "abcd0");
  }
}

//------------------------------------------------------------------------------

void test_opt() {
  {
    TextSpan text;
    TextSpan tail;

    text = utils::to_span("abcd");
    tail = Opt<Atom<'a'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "bcd");

    text = utils::to_span("abcd");
    tail = Opt<Atom<'b'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "abcd");
  }

  {
    TextSpan head, tail;

    tail = utils::to_span("abcd");
    head = Opt<Atom<'a'>>::take(ctx, tail);
    TEST(head == "a" && tail == "bcd");

    tail = utils::to_span("abcd");
    head = Opt<Atom<'b'>>::take(ctx, tail);
    TEST(head == "" && tail == "abcd");
  }
}

//------------------------------------------------------------------------------

void test_any() {
  {
    TextSpan text;
    TextSpan tail;

    text = utils::to_span("");
    tail = Any<Atom<'a'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "");

    text = utils::to_span("aaaabbbb");
    tail = Any<Atom<'a'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "bbbb");

    text = utils::to_span("aaaabbbb");
    tail = Any<Atom<'b'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "aaaabbbb");
  }

  {
    TextSpan head, tail;

    tail = utils::to_span("");
    head = Any<Atom<'a'>>::take(ctx, tail);
    TEST(head == "" && tail == "");

    tail = utils::to_span("aaaabbbb");
    head = Any<Atom<'a'>>::take(ctx, tail);
    TEST(head == "aaaa" && tail == "bbbb");

    tail = utils::to_span("aaaabbbb");
    head = Any<Atom<'b'>>::take(ctx, tail);
    TEST(head == "" && tail == "aaaabbbb");
  }
}

//------------------------------------------------------------------------------

void test_some() {
  {
    TextSpan text;
    TextSpan tail;

    text = utils::to_span("");
    tail = Some<Atom<'a'>>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "");

    text = utils::to_span("aaaabbbb");
    tail = Some<Atom<'a'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "bbbb");

    text = utils::to_span("aaaabbbb");
    tail = Some<Atom<'b'>>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "aaaabbbb");
  }

  {
    TextSpan head, tail;

    tail = utils::to_span("");
    head = Some<Atom<'a'>>::take(ctx, tail);
    TEST(!head && tail == "");

    tail = utils::to_span("aaaabbbb");
    head = Some<Atom<'a'>>::take(ctx, tail);
    TEST(head == "aaaa" && tail == "bbbb");

    tail = utils::to_span("aaaabbbb");
    head = Some<Atom<'b'>>::take(ctx, tail);
    TEST(!head && tail == "aaaabbbb");
  }
}

//------------------------------------------------------------------------------

void test_and() {
  {
    TextSpan text;
    TextSpan tail;

    text = utils::to_span("");
    tail = And<Atom<'a'>>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "");

    text = utils::to_span("aaaabbbb");
    tail = And<Atom<'a'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "aaaabbbb");

    text = utils::to_span("aaaabbbb");
    tail = And<Atom<'b'>>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "aaaabbbb");
  }

  {
    TextSpan head, tail;

    tail = utils::to_span("");
    head = And<Atom<'a'>>::take(ctx, tail);
    TEST(!head && tail == "");

    tail = utils::to_span("aaaabbbb");
    head = And<Atom<'a'>>::take(ctx, tail);
    TEST(head == "" && tail == "aaaabbbb");

    tail = utils::to_span("aaaabbbb");
    head = And<Atom<'b'>>::take(ctx, tail);
    TEST(!head && tail == "aaaabbbb");
  }
}

//------------------------------------------------------------------------------

void test_not() {
  {
    TextSpan text;
    TextSpan tail;

    text = utils::to_span("");
    tail = Not<Atom<'a'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "");

    text = utils::to_span("aaaabbbb");
    tail = Not<Atom<'a'>>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "aaaabbbb");

    text = utils::to_span("aaaabbbb");
    tail = Not<Atom<'b'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "aaaabbbb");
  }
  {
    TextSpan head, tail;

    tail = utils::to_span("");
    head = Not<Atom<'a'>>::take(ctx, tail);
    TEST(head == "" && tail == "");

    tail = utils::to_span("aaaabbbb");
    head = Not<Atom<'a'>>::take(ctx, tail);
    TEST(!head && tail == "aaaabbbb");

    tail = utils::to_span("aaaabbbb");
    head = Not<Atom<'b'>>::take(ctx, tail);
    TEST(head == "" && tail == "aaaabbbb");
  }
}

//------------------------------------------------------------------------------

void test_rep() {
  {
    TextSpan text;
    TextSpan tail;

    text = utils::to_span("");
    tail = Rep<3, Atom<'a'>>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "");

    text = utils::to_span("aabbbb");
    tail = Rep<3, Atom<'a'>>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "bbbb");

    text = utils::to_span("aaabbbb");
    tail = Rep<3, Atom<'a'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "bbbb");

    text = utils::to_span("aaaabbbb");
    tail = Rep<3, Atom<'a'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "abbbb");
  }

  {
    TextSpan head, tail;

    tail = utils::to_span("");
    head = Rep<3, Atom<'a'>>::take(ctx, tail);
    TEST(!head && tail == "");

    tail = utils::to_span("aabbbb");
    head = Rep<3, Atom<'a'>>::take(ctx, tail);
    TEST(!head && tail == "aabbbb");

    tail = utils::to_span("aaabbbb");
    head = Rep<3, Atom<'a'>>::take(ctx, tail);
    TEST(head == "aaa" && tail == "bbbb");

    tail = utils::to_span("aaaabbbb");
    head = Rep<3, Atom<'a'>>::take(ctx, tail);
    TEST(head == "aaa" && tail == "abbbb");
  }
}

//------------------------------------------------------------------------------

void test_reprange() {
  {
    TextSpan text;
    TextSpan tail;

    text = utils::to_span("bbbb");
    tail = RepRange<2, 3, Atom<'a'>>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "bbbb");

    text = utils::to_span("abbbb");
    tail = RepRange<2, 3, Atom<'a'>>::match(ctx, text);
    TEST(!tail.is_valid() && std::string(tail.end) == "bbbb");

    text = utils::to_span("aabbbb");
    tail = RepRange<2, 3, Atom<'a'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "bbbb");

    text = utils::to_span("aaabbbb");
    tail = RepRange<2, 3, Atom<'a'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "bbbb");

    text = utils::to_span("aaaabbbb");
    tail = RepRange<2, 3, Atom<'a'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "abbbb");
  }

  {
    TextSpan head, tail;

    tail = utils::to_span("bbbb");
    head = RepRange<2, 3, Atom<'a'>>::take(ctx, tail);
    TEST(!head && tail == "bbbb");

    tail = utils::to_span("abbbb");
    head = RepRange<2, 3, Atom<'a'>>::take(ctx, tail);
    TEST(!head && tail == "abbbb");

    tail = utils::to_span("aabbbb");
    head = RepRange<2, 3, Atom<'a'>>::take(ctx, tail);
    TEST(head == "aa" && tail == "bbbb");

    tail = utils::to_span("aaabbbb");
    head = RepRange<2, 3, Atom<'a'>>::take(ctx, tail);
    TEST(head == "aaa" && tail == "bbbb");

    tail = utils::to_span("aaaabbbb");
    head = RepRange<2, 3, Atom<'a'>>::take(ctx, tail);
    TEST(head == "aaa" && tail == "abbbb");
  }
}

//------------------------------------------------------------------------------

void test_until() {
  {
    TextSpan text;
    TextSpan tail;

    text = utils::to_span("");
    tail = Until<Atom<'b'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "");

    text = utils::to_span("aaaa");
    tail = Until<Atom<'b'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "");

    text = utils::to_span("aaaabbbb");
    tail = Until<Atom<'b'>>::match(ctx, text);
    TEST(tail.is_valid() && tail == "bbbb");
  }

  {
    TextSpan head, tail;

    tail = utils::to_span("");
    head = Until<Atom<'b'>>::take(ctx, tail);
    TEST(head == "" && tail == "");

    tail = utils::to_span("bbbb");
    head = Until<Atom<'b'>>::take(ctx, tail);
    TEST(head == "" && tail == "bbbb");

    tail = utils::to_span("aaaa");
    head = Until<Atom<'b'>>::take(ctx, tail);
    TEST(head == "aaaa" && tail == "");

    tail = utils::to_span("aaaabbbb");
    head = Until<Atom<'b'>>::take(ctx, tail);
    TEST(head == "aaaa" && tail == "bbbb");
  }
}


//------------------------------------------------------------------------------

TextSpan test_matcher(TextMatchContext& ctx, TextSpan body) {
  matcheroni_assert(body.is_valid());
  if (body.is_empty()) return body.fail();
  return body.begin[0] == 'a' ? body.advance(1) : body.fail();
}

TextSpan test_taker(TextMatchContext& ctx, TextSpan& body) {
  matcheroni_assert(body.is_valid());
  if (body.is_empty()) return body.fail();
  if (body.begin[0] == 'a') {
    TextSpan result = {body.begin, body.begin + 1};
    auto _ = body.take(1);
    return result;
  }
  else {
    return body.fail();
  }
}

void test_ref() {
  TextSpan text;
  TextSpan tail;

  text = utils::to_span("");
  tail = Ref<test_matcher>::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == "");

  text = utils::to_span("abc");
  tail = Ref<test_matcher>::match(ctx, text);
  TEST(tail.is_valid() && tail == "bc");

  text = utils::to_span("xyz");
  tail = Ref<test_matcher>::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == "xyz");

  {
    TextSpan head, tail;

    tail = utils::to_span("");
    head = Ref2<test_taker>::take(ctx, tail);
    TEST(!head && tail == "");
  }
}

//------------------------------------------------------------------------------

void test_backref() {
  TextSpan text;
  TextSpan tail;

  using pattern1 =
      Seq<StoreBackref<"backref", char, Rep<4, Range<'a', 'z'>>>,
          Atom<'-'>,
          MatchBackref<"backref", char, Rep<4, Range<'a', 'z'>>>>;

  text = utils::to_span("abcd-abcd!");
  tail = pattern1::match(ctx, text);
  TEST(tail.is_valid() && tail == "!");

  text = utils::to_span("zyxw-zyxw!");
  tail = pattern1::match(ctx, text);
  TEST(tail.is_valid() && tail == "!");

  text = utils::to_span("abcd-abqd!");
  tail = pattern1::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == "qd!");

  text = utils::to_span("abcd-abc");
  tail = pattern1::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == "");

  text = utils::to_span("ab01-ab01!");
  tail = pattern1::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == "01-ab01!");
}

//------------------------------------------------------------------------------

void test_delimited_block() {
  TextSpan text;
  TextSpan tail;

  using pattern = DelimitedBlock<Atom<'{'>, Atom<'a'>, Atom<'}'>>;

  text = utils::to_span("{}bbbb");
  tail = pattern::match(ctx, text);
  TEST(tail.is_valid() && tail == "bbbb");

  text = utils::to_span("{a}bbbb");
  tail = pattern::match(ctx, text);
  TEST(tail.is_valid() && tail == "bbbb");

  text = utils::to_span("{aa}bbbb");
  tail = pattern::match(ctx, text);
  TEST(tail.is_valid() && tail == "bbbb");

  text = utils::to_span("{bb}aaaa");
  tail = pattern::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == "bb}aaaa");

  text = utils::to_span("{aabb}bbbb");
  tail = pattern::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == "bb}bbbb");

  text = utils::to_span("{aaaa");
  tail = pattern::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == "");
}

//------------------------------------------------------------------------------

void test_delimited_list() {
  TextSpan text;
  TextSpan tail;

  using pattern = DelimitedList<Atom<'{'>, Atom<'a'>, Atom<','>, Atom<'}'>>;

  // Zero items
  text = utils::to_span("{}bbbb");
  tail = pattern::match(ctx, text);
  TEST(tail.is_valid() && tail == "bbbb");

  // One item
  text = utils::to_span("{a}bbbb");
  tail = pattern::match(ctx, text);
  TEST(tail.is_valid() && tail == "bbbb");

  // Two items
  text = utils::to_span("{a,a}bbbb");
  tail = pattern::match(ctx, text);
  TEST(tail.is_valid() && tail == "bbbb");

  // Two items + trailing comma
  text = utils::to_span("{a,a,}bbbb");
  tail = pattern::match(ctx, text);
  TEST(tail.is_valid() && tail == "bbbb");

  // Ldelim missing
  text = utils::to_span("a,a}bbbb");
  tail = pattern::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == "a,a}bbbb");

  // Item missing
  text = utils::to_span("{,a}bbbb");
  tail = pattern::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == ",a}bbbb");

  // Separator missing
  text = utils::to_span("{aa}bbbb");
  tail = pattern::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == "a}bbbb");

  // Rdelim missing
  text = utils::to_span("{a,abbbb");
  tail = pattern::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == "bbbb");

  // Wrong separator
  text = utils::to_span("{a;a}bbbb");
  tail = pattern::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == ";a}bbbb");

  // Wrong item
  text = utils::to_span("{a,b}bbbb");
  tail = pattern::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == "b}bbbb");
}

//------------------------------------------------------------------------------

void test_eol() {
  TextSpan text;
  TextSpan tail;

  text = utils::to_span("aaaa");
  tail = Seq<Some<Atom<'a'>>, EOL>::match(ctx, text);
  TEST(tail.is_valid() && tail == "");

  text = utils::to_span("aaaabbbb");
  tail = Seq<Some<Atom<'a'>>, EOL>::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == "bbbb");

  text = utils::to_span("aaaa\nbbbb");
  tail = Seq<Some<Atom<'a'>>, EOL>::match(ctx, text);
  TEST(tail.is_valid() && tail == "\nbbbb");
}

//------------------------------------------------------------------------------

void test_charset() {
  TextSpan text;
  TextSpan tail;

  text = utils::to_span("dcbaxxxx");
  tail = Some<Charset<"abcd">>::match(ctx, text);
  TEST(tail.is_valid() && tail == "xxxx");

  text = utils::to_span("xxxxabcd");
  tail = Some<Charset<"abcd">>::match(ctx, text);
  TEST(!tail.is_valid() && std::string(tail.end) == "xxxxabcd");

  text = utils::to_span("ddccxxxxbbaa");
  tail = Some<Charset<"abcd">>::match(ctx, text);
  TEST(tail.is_valid() && tail == "xxxxbbaa");
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
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
  test_not();
  test_rep();
  test_reprange();
  test_until();
  test_ref();
  test_backref();
  test_delimited_block();
  test_delimited_list();
  test_eol();
  test_charset();

  if (fail_count) printf("Failed %d tests!\n", fail_count);

  return fail_count ? -1 : 0;
}

//------------------------------------------------------------------------------
