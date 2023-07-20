//------------------------------------------------------------------------------
// Matcheroni is a single-header, zero-dependency toolkit that makes building
// custom pattern matchers, lexers, and parsers easier. Matcheroni is based on
// "Parsing Expression Grammars" (PEGs), which are similar in concept to
// regular expressions but behave slightly differently.

// See https://en.wikipedia.org/wiki/Parsing_expression_grammar for details.

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include <stdio.h>   // for size_t
#include <assert.h>  // for assert

// Assertions can actually speed things up as they mark code paths as dead
#define matcheroni_assert(A) assert(A)
//#define matcheroni_assert(A)

// FIXME make sure everything is tail-call-optimizable

namespace matcheroni {

//------------------------------------------------------------------------------
// Matcheroni operates on spans of atoms, usually (but not always) characters.
// Valid spans have non-null a and b pointers, empty spans have equal non-null
// a and b pointers.

template <typename atom>
struct Span {
  using AtomType = atom;

  constexpr Span() : a(nullptr), b(nullptr) {}
  constexpr Span(const atom* a, const atom* b) : a(a), b(b) {}

  size_t len() const {
    matcheroni_assert(is_valid());
    return b - a;
  }

  //----------------------------------------

  bool operator==(const Span& c) const {
    return a == c.a && b == c.b;
  }

  bool operator==(const char* text) {
    matcheroni_assert(is_valid());
    return strcmp_span(*this, text) == 0;
  }

  bool is_empty() const { return a && a == b; }
  bool is_valid() const { return a; }

  // this must be the same as is_valid() otherwise
  // if (auto end = match()) {}
  // fails if end is EOF
  operator bool() const { return a; }

  //----------------------------------------

  [[nodiscard]] Span fail() const {
    return a ? Span(nullptr, a) : Span(a, b);
  }

  [[nodiscard]] Span advance(int offset) const {
    matcheroni_assert(a);
    return {a + offset, b};
  }

  //----------------------------------------

  const atom* a;
  const atom* b;
};

//------------------------------------------------------------------------------
// Matchers will often need to compare spans against null-delimited strings ala
// strcmp(), so we provide this function for convenience.

inline int strcmp_span(const Span<char>& s2, const char* lit) {
  Span<char> s = s2;
  while (1) {
    auto ca = s.a == s.b ? 0 : *s.a;
    auto cb = *lit;
    if (ca != cb || ca == 0) return ca - cb;
    s.a++;
    lit++;
  }
}

inline int strcmp_span(const Span<char>& a, const Span<char>& b) {
  if (int c = a.len() - b.len()) return c;
  for (size_t i = 0; i < a.len(); i++) {
    if (auto c = a.a[i] - b.a[i]) return c;
  }
  return 0;
}

//------------------------------------------------------------------------------
// Matcheroni is based on building trees of "matcher" functions. A matcher
// function takes a span of "atoms" (could be characters, could be some
// application-specific type) as input and returns either a span containing the
// _remaining_text_ if a match was found, or a span containing a nullptr and
// the point at which the match failed.

// Matcher functions accept a "context" object which stores persistent state
// across matcher calls.

// Matcher functions can also be member functions _of_ the context object,
// which works equally well.

template <typename context, typename atom>
using matcher_function = Span<atom> (*)(context& ctx, Span<atom> s);

//------------------------------------------------------------------------------
// Matchers require a context object to perform two essential functions -
// compare atoms and rewind any internal state when a partial match fails.

// Since we will be matching text 99% of the time, this context class provides
// the minimal amount of code needed to run and debug Matcheroni patterns.

struct TextContext {

  static int atom_cmp(char a, int b) { return a - b; }

  // Rewind does nothing as it doesn't interact with trace_depth.
  template<typename atom> void rewind(Span<atom> s) {}

  // Tracing requires us to keep track of the nesting depth in the context.
  int trace_depth = 0;
};

using TextSpan = Span<char>;

//------------------------------------------------------------------------------
// Matcheroni consists of a base set of matcher functions wrapped in templated
// structs. Wrapping them this way allows us to compose functions using
// nested templates:
//
// using pattern = Seq<Atom<'f'>, Atom<'o'>, Atom<'o'>>;
// TextSpan end = pattern::match(ctx, text);

// The most fundamental unit of matching is a single atom. For convenience, we
// implement the Atom matcher so that it can handle small sets of atoms.

// Examples:
// Atom<'a'>::match("abcd"...) == "bcd"
// Atom<'a','c'>::match("cdef"...) == "def"

template <auto... rest>
struct Atom;

template <auto C, auto... rest>
struct Atom<C, rest...> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    if (ctx.atom_cmp(*s.a, C) == 0) {
      return s.advance(1);
    } else {
      return Atom<rest...>::match(ctx, s);
    }
  }
};

template <auto C>
struct Atom<C> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    if (ctx.atom_cmp(*s.a, C) == 0) {
      return s.advance(1);
    } else {
      return s.fail();
    }
  }
};

//------------------------------------------------------------------------------
// 'NotAtom' matches any atom that is _not_ in its argument list, which is a
// bit faster than using Seq<Not<Atom<...>>, AnyAtom>

template <auto C, auto... rest>
struct NotAtom {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    if (ctx.atom_cmp(*s.a, C) == 0) {
      return s.fail();
    }
    return NotAtom<rest...>::match(ctx, s);
  }
};

template <auto C>
struct NotAtom<C> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    if (ctx.atom_cmp(*s.a, C) == 0) {
      return s.fail();
    } else {
      return s.advance(1);
    }
  }
};

//------------------------------------------------------------------------------
// AnyAtom is equivalent to '.' in regex.

struct AnyAtom {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();
    return s.advance(1);
  }
};

//------------------------------------------------------------------------------
// 'Range' matches ranges of atoms, equivalent to '[a-b]' in regex.

template <auto RA, decltype(RA) RB, auto... rest>
struct Range {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    if ((ctx.atom_cmp(*s.a, RA) >= 0) && (ctx.atom_cmp(*s.a, RB) <= 0)) {
      return s.advance(1);
    }
    return Range<rest...>::match(ctx, s);
  }
};

template <auto RA, decltype(RA) RB>
struct Range<RA, RB> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    if ((ctx.atom_cmp(*s.a, RA) >= 0) && (ctx.atom_cmp(*s.a, RB) <= 0)) {
      return s.advance(1);
    }
    return s.fail();
  }
};

//------------------------------------------------------------------------------
// 'NotRange' matches ranges of atoms not in the given range, equivalent to
// '[^a-b]' in regex.

template <auto RA, decltype(RA) RB, auto... rest>
struct NotRange {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    if ((ctx.atom_cmp(*s.a, RA) >= 0) && (ctx.atom_cmp(*s.a, RB) <= 0)) {
      return s.fail();
    }

    return NotRange<rest...>::match(ctx, s);
  }
};

template <auto RA, decltype(RA) RB>
struct NotRange<RA, RB> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    if ((ctx.atom_cmp(*s.a, RA) >= 0) && (ctx.atom_cmp(*s.a, RB) <= 0)) {
      return s.fail();
    }

    return s.advance(1);
  }
};

//------------------------------------------------------------------------------
// Not a matcher, but a template helper that allows us to use strings as
// template parameters. The parameter behaves as a fixed-length character array
// that does ___NOT___ match the trailing null.

template <int N>
struct StringParam {
  constexpr StringParam(const char (&str)[N]) {
    for (int i = 0; i < N; i++) str_val[i] = str[i];
  }
  constexpr static auto str_len = N - 1;
  char str_val[N];

  Span<char> span() const {
    return Span<char>(str_val, str_val + str_len);
  }
};

//------------------------------------------------------------------------------
// 'Lit' matches string literals. Does ___NOT___ match the trailing null.

// Lit<"foo">::match("foobar") == "bar"

template <typename Context, typename SpanType>
inline SpanType match_lit(Context& ctx, SpanType s, const char* lit, size_t len) {
  matcheroni_assert(s.is_valid());
  if (len > s.len()) return s.fail();

  for (size_t i = 0; i < len; i++) {
    if (ctx.atom_cmp(*s.a, *lit)) return s.fail();
    s = s.advance(1);
    lit++;
  }
  return s;
}

template <StringParam lit>
struct Lit {
  template <typename Context, typename SpanType>
  static SpanType match(Context& ctx, SpanType s) {
    return match_lit(ctx, s, lit.str_val, lit.str_len);
  }
};


//------------------------------------------------------------------------------
// 'Seq' (sequence) succeeds if all of its sub-matchers succeed in order.

// Examples:
// Seq<Atom<'a'>, Atom<'b'>::match("abcd") == "cd"

template <typename P, typename... rest>
struct Seq {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    Span<atom> end = P::match(ctx, s);
    if (!end.is_valid()) return end;
    return Seq<rest...>::match(ctx, end);

    // FIXME would this be faster?
    //if (!s) return s;
    //return Seq<rest...>::match(ctx, P::match(ctx, s));
  }
};

template <typename P>
struct Seq<P> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    return P::match(ctx, s);

    // FIXME would this be faster?
    //if (!s) return s;
    //return P::match(ctx, s);
  }
};


//------------------------------------------------------------------------------
// 'Oneof' returns the first match in a set of matchers, equivalent to (A|B|C)
// in regex.

// Examples:
// Oneof<Atom<'a'>, Atom<'b'>>::match("abcd") == "bcd"
// Oneof<Atom<'a'>, Atom<'b'>>::match("bcde") == "cde"

template <typename P, typename... rest>
struct Oneof {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    auto c = P::match(ctx, s);
    if (c.is_valid()) {
      return c;
    }

    /*+*/ ctx.rewind(s);
    auto d = Oneof<rest...>::match(ctx, s);

    if (d.is_valid()) {
      return d;
    } else {
      // Both attempts failed, return whichever match got farther.
      return c.b > d.b ? c : d;
    }
  }
};

template <typename P>
struct Oneof<P> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    return P::match(ctx, s);
  }
};

//------------------------------------------------------------------------------
// Matches exactly one instance of P. Yes, this is effectively a do-nothing
// matcher. It exists only to make things like the pattern below read better.

// using pattern =
// Seq<
//   Any <A>,
//   Opt <B>,
//   One <C>,
//   Some<D>,
//   One <E>
// >;

template<typename P>
struct One {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    return P::match(ctx, s);
  }
};

//------------------------------------------------------------------------------
// 'Opt' matches 'optional' patterns, equivalent to '?' in regex.

// Opt<Atom<'a'>>::match("abcd") == "bcd"
// Opt<Atom<'a'>>::match("bcde") == "bcde"

template <typename... rest>
struct Opt {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    auto c = Oneof<rest...>::match(ctx, s);
    if (c.is_valid()) {
      return c;
    } else {
      /*+*/ ctx.rewind(s);
      return s;
    }
  }
};

//------------------------------------------------------------------------------
// 'Any' matches zero or more copies of a pattern, equivalent to '*' in regex.

// HOWEVER - Seq<Any<Atom<'a'>>, Atom<'a'>> (unlike "a*a" in regex) will
// _never_ match anything, as the first Any<> is greedy and consumes all 'a's
// without doing any backtracking.

// Any<Atom<'a'>>::match("aaaab") == "b"
// Any<Atom<'a'>>::match("bbbbc") == "bbbbc"

template <typename... rest>
struct Any {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s;

    while (1) {
      auto c = Oneof<rest...>::match(ctx, s);
      if (!c.is_valid()) break;
      s = c;
    }

    // Fixme Does this _really_ need a rewind?
    /*+*/ ctx.rewind(s);

    return s;
  }
};

//------------------------------------------------------------------------------
// Nothing always succeeds in matching nothing. Makes a good placeholder. :)

struct Nothing {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    return s;
  }
};

//------------------------------------------------------------------------------
// 'Some' matches one or more copies of a pattern, equivalent to '+' in regex.

// Some<Atom<'a'>>::match("aaaab") == "b"
// Some<Atom<'a'>>::match("bbbbc") == nullptr

template <typename... rest>
struct Some {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    auto c = Any<rest...>::match(ctx, s);
    return (c == s) ? s.fail() : c;
  }
};

//------------------------------------------------------------------------------
// The 'And' predicate matches a pattern but does _not_ advance the cursor.
// Used for lookahead.

// And<Atom<'a'>>::match("abcd") == "abcd"
// And<Atom<'a'>>::match("bcde") == nullptr

template <typename P>
struct And {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    auto c = P::match(ctx, s);
    /*+*/ ctx.rewind(s);
    return c.is_valid() ? s : c;
  }
};

//------------------------------------------------------------------------------
// The 'Not' predicate is the logical negation of the 'And' predicate.

// Not<Atom<'a'>>::match("abcd") == nullptr
// Not<Atom<'a'>>::match("bcde") == "bcde"

template <typename P>
struct Not {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    auto c = P::match(ctx, s);
    /*+*/ ctx.rewind(s);
    return c.is_valid() ? s.fail() : s;
  }
};

//------------------------------------------------------------------------------
// 'SeqOpt' matches a sequence of items that are individually optional, but
// that must match in order if present.

// SeqOpt<Atom<'a'>, Atom<'b'>, Atom<'c'>> will match "a", "ab", and "abc" but
// not "bc" or "c".

template <typename P, typename... rest>
struct SeqOpt {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    auto c = P::match(ctx, s);
    if (c.is_valid()) {
      s = c;
    }

    return SeqOpt<rest...>::match(ctx, s);
  }
};

template <typename P>
struct SeqOpt<P> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    auto c = P::match(ctx, s);
    if (c.is_valid()) {
      s = c;
    }

    return s;
  }
};

//------------------------------------------------------------------------------
// 'NotEmpty' turns empty sequence matches into non-matches. Useful if you have
// "a OR b OR ab" patterns, as you can turn them into NonEmpty<Opt<A>, Opt<B>>.

// NotEmpty<Opt<Atom<'c'>>, Opt<Atom<'d'>>>::match("cq") == "q"
// NotEmpty<Opt<Atom<'c'>>, Opt<Atom<'d'>>>::match("dq") == "q"
// NotEmpty<Opt<Atom<'c'>>, Opt<Atom<'d'>>>::match("zq") == nullptr

template <typename... rest>
struct NotEmpty {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    auto end = Seq<rest...>::match(ctx, s);
    return end == s ? s.fail() : end;
  }
};

//------------------------------------------------------------------------------
// 'Rep' is equivalent to '{N}' in regex.

template <int N, typename P>
struct Rep {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    for (auto i = 0; i < N; i++) {
      s = P::match(ctx, s);
      if (!s.is_valid()) break;
    }
    return s;
  }
};

//------------------------------------------------------------------------------
// 'RepRange' is equivalent '{M,N}' in regex.

template <int M, int N, typename P>
struct RepRange {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    s = Rep<M, P>::match(ctx, s);
    if (!s.is_valid()) return s;

    for (auto i = 0; i < N - M; i++) {
      auto c = P::match(ctx, s);
      if (!c.is_valid()) break;
      s = c;
    }

    return s;
  }
};

//------------------------------------------------------------------------------
// 'Until' matches anything until we see the given pattern or we hit EOF.
// The pattern is _not_ consumed.

// Equivalent to Any<Seq<Not<M>,AnyAtom>>


template<typename P>
struct Until {
  template<typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    while(1) {
      if (s.is_empty()) return s;
      auto end = P::match(ctx, s);
      if (end.is_valid()) {
        ctx.rewind(s);
        return s;
      }
      s = s.advance(1);
    }
  }
};

#if 0
// These patterns need better names and explanations...

// Match some P until we see T. No T = no match.
template<typename P, typename T>
struct SomeUntil {
  template<typename atom>
  static atom* match(context& ctx, Span<atom> s) {
    auto c = AnyUntil<P,T>::match(ctx, a, b);
    return c == a ? nullptr : c;
  }
};

// Match any P until we see T. No T = no match.
template<typename P, typename T>
struct AnyUntil {
  template<typename atom>
  static atom* match(context& ctx, Span<atom> s) {
    while(a < b) {
      if (T::match(ctx, a, b)) return a;
      if (auto end = P::match(ctx, a, b)) {
        a = end;
      }
      else {
        return s.fail();
      }
    }
    return s.fail();
  }
};

// Match some P unless it would match T.
template<typename P, typename T>
struct SomeUnless {
  template<typename atom>
  static atom* match(context& ctx, Span<atom> s) {
    auto c = AnyUnless<P,T>::match(ctx, a, b);
    return c == a ? s.fail() : {c, a};
  }
};

// Match any P unless it would match T.
template<typename P, typename T>
struct AnyUnless {
  template<typename atom>
  static atom* match(context& ctx, Span<atom> s) {
    while(a < b) {
      if (T::match(ctx, a, b)) return a;
      if (auto end = P::match(ctx, a, b)) {
        a = end;
      }
      else {
        return {a, a};
      }
    }
    return s.fail();
  }
};
#endif

//------------------------------------------------------------------------------
// 'Ref' is used to call a user-defined matcher function from a Matcheroni
// pattern.

// 'Ref' can also be used to call member functions. You _MUST_ pass a pointer
// to an object via the 'ctx' parameter when using these.

// Span<atom> my_special_matcher(context& ctx, Span<atom> s);
// using pattern = Ref<my_special_matcher>;

// struct Foo {
//   Span<atom> match(Span<atom> s);
// };
//
// using pattern = Ref<&Foo::match>;
// Foo my_foo;
// auto end = pattern::match(&my_foo, s);

template <auto F>
struct Ref;

template <typename context, typename atom, Span<atom> (*func)(context& ctx, Span<atom> s)>
struct Ref<func> {
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    return func(ctx, s);
  }
};

template <typename context, typename atom, Span<atom> (context::*func)(Span<atom> s)>
struct Ref<func> {
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    return (ctx.*func)(s);
  }
};

//------------------------------------------------------------------------------
// 'StoreBackref/MatchBackref' stores and matches backreferences.
// These are currently used for raw string delimiters in the C lexer.

// Note that the backreference is stored as a static pointer in the
// StoreBackref template, so be careful of nesting as you could clobber it.

// FIXME this should create a temp node or something like the bookmarks

template <StringParam name, typename atom, typename P>
struct StoreBackref {
  inline static Span<atom> ref;

  template<typename context>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    auto c = P::match(ctx, s);
    if (!c.is_valid()) {
      ref = s.fail();
      return c;
    }
    ref = {s.a, c.a};

    //printf("Backref: `");
    //for (auto d = s.a; d < c.a; d++) putc(*d, stdout);
    //printf("`\n");
    return c;
  }
};

template <StringParam name, typename atom, typename P>
struct MatchBackref {

  template<typename context>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    auto ref = StoreBackref<name, atom, P>::ref;
    if (!ref.is_valid()) return s.fail();

    for (size_t i = 0; i < ref.len(); i++) {
      if (s.is_empty()) return s.fail();
      if (ctx.atom_cmp(*s.a, ref.a[i])) return s.fail();
      s = s.advance(1);
    }

    return s;
  }
};

//------------------------------------------------------------------------------
// 'DelimitedBlock' is equivalent to Seq<ldelim, Any<body>, rdelim>, but it
// tries to match rdelim before body which can save matching time.

template <typename ldelim, typename body, typename rdelim>
struct DelimitedBlock {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    s = ldelim::match(ctx, s);
    if (!s.is_valid()) return s;

    while (1) {
      auto end = rdelim::match(ctx, s);
      if (end.is_valid()) return end;
      s = body::match(ctx, s);
      if (!s.is_valid()) break;
    }
    return s;
  }
};

//------------------------------------------------------------------------------
// 'DelimitedList' is the same as 'DelimitedBlock' except that it adds a
// separator pattern between items.

template <typename ldelim, typename item, typename separator, typename rdelim>
struct DelimitedList {
  // Might be faster to do this in terms of comma_separated, etc?

  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    s = ldelim::match(ctx, s);
    if (!s.is_valid()) return s;

    while (1) {
      auto end1 = rdelim::match(ctx, s);
      if (end1.is_valid()) return end1;
      s = item::match(ctx, s);
      if (!s.is_valid()) break;

      auto end2 = rdelim::match(ctx, s);
      if (end2.is_valid()) return end2;
      s = separator::match(ctx, s);
      if (!s.is_valid()) break;
    }
    return s;
  }
};

//------------------------------------------------------------------------------
// 'EOL' matches newline and EOF, but does not advance past it.

struct EOL {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s;
    if (s.a[0] == atom('\n')) return s;
    return s.fail();
  }
};

//------------------------------------------------------------------------------
// 'Search' is not a matcher, just a convenience helper - searches for a
// pattern anywhere in the input span and returns offset/length of the match if
// found.

#if 0
// broken
struct SearchResult {
  operator bool() const { return length > 0; }
  int offset;
  int length;
};

template<typename P>
struct Search {
  template<typename atom>
  static SearchResult search(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    auto cursor = s;
    while(1) {
      auto end = P::match(ctx, cursor, b);
      if (end) {
        return {cursor-a, end-cursor};
      }
      cursor++;
      if (cursor == b) return {0,0};
    }
  }
};
#endif

//------------------------------------------------------------------------------
// 'Charset' matches larger sets of atoms packed into a string literal, which
// is more concise than Atom<'a','b','c'...> for large sets of atoms.

// Charset<"abcdef">::match("defg") == "efg"

template <StringParam chars>
struct Charset {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    for (auto i = 0; i < chars.str_len; i++) {
      if (ctx.atom_cmp(s.a[0], chars.str_val[i]) == 0) {
        return s.advance(1);
      }
    }
    return s.fail();
  }
};

//------------------------------------------------------------------------------
// 'Map' returns the result of the first matcher whose 'match_key' matches the
// input. Unlike 'Oneof', if the key pattern matches but the value pattern does
// not, the rest of the options will _not_ be checked. This can make matching
// much faster by allowing large arrays of options to be broken down into
// skippable groups.

// WARNING - I haven't tested this at all, I'm not sure if it's even a
// performance improvement in most cases.

// Note - the key is _NOT_ consumed, as the key pattern may be substantially
// different than the match pattern (for example matching a single character as
// a key for a match pattern consisting of a bunch of operators starting with
// that character)

// Example:

// using Declaration =
// Map<
//   KeyVal<Atom<"struct">, NodeStruct>,
//   KeyVal<Atom<"union">,  NodeUnion>,
//   KeyVal<Atom<"class",   NodeClass>,
//   KeyVal<Atom<"enum",    NodeEnum>,
//   KeyVal<AnyAtom,        NodeVariable>
// >;

#if 0
template<typename P, typename... rest>
struct Map {
  template<typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    if (P::match_key(a, b)) {
      ctx.rewind(s);
      return P::match(ctx, a, b);
    }
    else {
      ctx.rewind(s);
      return Map<rest...>::match(ctx, a, b);
    }
  }
};

template <typename P>
struct Map<P> {
  template<typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    return P::match(ctx, a, b);
  }
};

template <typename K, typename V>
struct KeyVal {
  template<typename atom>
  static Span<atom> match_key(context& ctx, Span<atom> s) {
    return K::match(ctx, a, b);
  }

  template<typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    return V::match(ctx, a, b);
  }
};
#endif

//------------------------------------------------------------------------------
// 'PatternWrapper' is just a convenience class that lets you do this:

// struct MyPattern : public PatternWrapper<MyPattern> {
//   using pattern = Atom<'a', 'b', 'c'>;
// };
//
// auto end = Some<MyPattern>::match(ctx, a, b);

template <typename T>
struct PatternWrapper {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> s) {
    return T::pattern::match(ctx, s);
  }
};

//------------------------------------------------------------------------------

};  // namespace matcheroni
