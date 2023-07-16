//------------------------------------------------------------------------------
// Matcheroni is a single-header, zero-dependency toolkit that makes building
// custom pattern matchers, lexers, and parsers easier. Matcheroni is based on
// "Parsing Expression Grammars" (PEGs), which are similar in concept to
// regular expressions but behave slightly differently.

// See https://en.wikipedia.org/wiki/Parsing_expression_grammar for details.

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include <stdio.h>
#include <assert.h>

// Assertions can actually speed things up as they mark code paths as dead
#define matcheroni_assert(A) assert(A)
//#define matcheroni_assert(A)

namespace matcheroni {

//------------------------------------------------------------------------------

template <typename atom>
struct Span {
  using AtomType = atom;

  Span() : a(nullptr), b(nullptr) {}

  Span(const atom* a, const atom* b) : a(a), b(b) {
    if (a == nullptr) {
      int x = 1;
      x++;
    }
  }

  Span advance(int offset) const {
    matcheroni_assert(a);
    return {a + offset, b};
  }

  Span operator-(Span y) const {
    auto x = *this;

    if (x.a == y.a && x.b >= y.b) {
      // chop the front off
      return Span(y.b, x.b);
    }

    if (x.b == y.b && x.a <= y.a) {
      // chop the back off
      return Span(x.a, y.a);
    }

    matcheroni_assert(false);
    return fail();
  }

  bool operator==(const Span& c) const { return a == c.a && b == c.b; }

  bool is_empty() const { return a && a == b; }
  bool is_valid() const { return a; }

  operator bool() const { return a && b && a < b; }

  template<typename atom2>
  operator Span<const atom2>() const {
    return Span<const atom2>(a, b);
  }

  Span fail() const {
    return a ? Span(nullptr, a) : Span(a, b);
  }

  size_t len() const {
    matcheroni_assert(a);
    return b - a;
  }

  const atom* a;
  const atom* b;
};

// We'll be using spans of constant characters a lot, so this is a convenience
// declaration.
using cspan = Span<const char>;

//------------------------------------------------------------------------------
// Matcheroni is based on building trees of "matcher" functions. A matcher
// function takes a span of "atoms" (could be characters, could be some
// application-specific type) as input and returns either a span containing the
// remaining text if a match was found, or a span containing a nullptr and the
// point at which the match failed.

// Matcher functions accept an opaque context pointer 'ctx', which can be used
// to pass in a pointer to application-specific state.

template <typename atom>
using matcher_function = Span<atom> (*)(void* ctx, Span<atom> s);

//------------------------------------------------------------------------------
// Matchers will often need to compare spans against null-delimited strings ala
// strcmp(), so we provide this function for convenience.

inline int strcmp_span(Span<const char> s, const char* lit) {
  while (1) {
    auto ca = s.a == s.b ? 0 : *s.a;
    auto cb = *lit;
    if (ca != cb || ca == 0) return ca - cb;
    s.a++;
    lit++;
  }
}

//------------------------------------------------------------------------------
// Matcheroni needs some way to compare atoms against constants. By default, it
// uses a generic atom_cmp function to compute the integer difference between
// the atom and the constant.

// If you specialize the function below for your various atom types and
// constant types, Matcheroni will use your code instead. Your atom_cmp()
// should return <0 for a<b, ==0 for a==b, and >0 for a>b.

template <typename atom1, typename atom2>
inline int atom_cmp(void* ctx, const atom1* a, const atom2 b) {
  return int(*a - b);
}

//------------------------------------------------------------------------------
// Matcheroni also needs a way to tell the host application to "rewind" its
// state when an intermediate match fails - this can be used to clean up any
// intermediate data structures that were created during the failed partial
// match.

// By default this does nothing, but if you specialize this for your atom type
// Matcheroni will call that instead.

template <typename atom>
inline void parser_rewind(void* ctx, Span<atom> s) {}

//------------------------------------------------------------------------------
// The most fundamental unit of matching is a single atom. For convenience, we
// implement the Atom matcher so that it can handle small sets of atoms.

// Examples:
// Atom<'a'>::match("abcd"...) == "bcd"
// Atom<'a','c'>::match("cdef"...) == "def"

template <auto... rest>
struct Atom;

template <auto C, auto... rest>
struct Atom<C, rest...> {
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    if (atom_cmp(ctx, s.a, C) == 0) {
      return s.advance(1);
    } else {
      return Atom<rest...>::match(ctx, s);
    }
  }
};

template <auto C>
struct Atom<C> {
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    if (atom_cmp(ctx, s.a, C) == 0) {
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
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    if (atom_cmp(ctx, s.a, C) == 0) {
      return s.fail();
    }
    return NotAtom<rest...>::match(ctx, s);
  }
};

template <auto C>
struct NotAtom<C> {
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    if (atom_cmp(ctx, s.a, C) == 0) {
      return s.fail();
    } else {
      return s.advance(1);
    }
  }
};

//------------------------------------------------------------------------------
// AnyAtom is equivalent to '.' in regex.

struct AnyAtom {
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();
    return s.advance(1);
  }
};

//------------------------------------------------------------------------------
// 'Range' matches ranges of atoms, equivalent to '[a-b]' in regex.

template <auto RA, decltype(RA) RB>
struct Range {
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();
    if (atom_cmp(ctx, s.a, RA) < 0) return s.fail();
    if (atom_cmp(ctx, s.a, RB) > 0) return s.fail();
    return s.advance(1);
  }
};

//------------------------------------------------------------------------------
// 'NotRange' matches ranges of atoms not in the given range, equivalent to
// '[^a-b]' in regex.

template <auto RA, decltype(RA) RB>
struct NotRange {
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();
    if (atom_cmp(ctx, s.a, RA) < 0) return s.advance(1);
    if (atom_cmp(ctx, s.a, RB) > 0) return s.advance(1);
    return s.fail();
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
  char str_val[str_len + 1];
};

//------------------------------------------------------------------------------
// 'Lit' matches string literals. Does ___NOT___ match the trailing null.

// Lit<"foo">::match("foobar") == "bar"

inline cspan match_lit(void* ctx, cspan s, const char* lit, size_t len) {
  matcheroni_assert(s.is_valid());
  if (len > s.len()) return s.fail();

  for (size_t i = 0; i < len; i++) {
    if (*s.a != *lit) return s.fail();
    s = s.advance(1);
    lit++;
  }
  return s;
}

template <StringParam lit>
struct Lit {
  static cspan match(void* ctx, cspan s) {
    return match_lit(ctx, s, lit.str_val, lit.str_len);
  }
};

//------------------------------------------------------------------------------
// 'Seq' (sequence) succeeds if all of its sub-matchers succeed in order.

// Examples:
// Seq<Atom<'a'>, Atom<'b'>::match("abcd") == "cd"

template <typename P, typename... rest>
struct Seq {
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    auto end = P::match(ctx, s);
    if (!end.is_valid()) return end;
    return Seq<rest...>::match(ctx, end);
  }
};

template <typename P>
struct Seq<P> {
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    return P::match(ctx, s);
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
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    auto c = P::match(ctx, s);
    if (c.is_valid()) {
      return c;
    }

    /*+*/ parser_rewind(ctx, s);
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
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    return P::match(ctx, s);
  }
};

//------------------------------------------------------------------------------
// 'Opt' matches 'optional' patterns, equivalent to '?' in regex.

// Opt<Atom<'a'>>::match("abcd") == "bcd"
// Opt<Atom<'a'>>::match("bcde") == "bcde"

template <typename... rest>
struct Opt {
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    auto c = Oneof<rest...>::match(ctx, s);
    if (c.is_valid()) {
      return c;
    } else {
      /*+*/ parser_rewind(ctx, s);
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
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s;

    while (1) {
      auto c = Oneof<rest...>::match(ctx, s);
      if (!c.is_valid()) break;
      s = c;
    }

    // Fixme Does this _really_ need a rewind?
    /*+*/ parser_rewind(ctx, s);

    return s;
  }
};

//------------------------------------------------------------------------------
// Nothing always succeeds in matching nothing. Makes a good placeholder. :)

struct Nothing {
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
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
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
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
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    auto c = P::match(ctx, s);
    /*+*/ parser_rewind(ctx, s);
    return c.is_valid() ? s : c;
  }
};

//------------------------------------------------------------------------------
// The 'Not' predicate is the logical negation of the 'And' predicate.

// Not<Atom<'a'>>::match("abcd") == nullptr
// Not<Atom<'a'>>::match("bcde") == "bcde"

template <typename P>
struct Not {
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    auto c = P::match(ctx, s);
    /*+*/ parser_rewind(ctx, s);
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
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    if (auto c = P::match(ctx, s)) {
      s = c;
    }

    return SeqOpt<rest...>::match(ctx, s);
  }
};

template <typename P>
struct SeqOpt<P> {
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    if (auto c = P::match(ctx, s)) {
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
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    auto end = Seq<rest...>::match(ctx, s);
    return end == s ? s.fail() : end;
  }
};

//------------------------------------------------------------------------------
// 'Rep' is equivalent to '{N}' in regex.

template <int N, typename P>
struct Rep {
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
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
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
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
  template<typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    while(1) {
      if (s.is_empty()) return s;
      auto end = P::match(ctx, s);
      if (end.is_valid()) {
        parser_rewind(ctx, s);
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
  static atom* match(void* ctx, Span<atom> s) {
    auto c = AnyUntil<P,T>::match(ctx, a, b);
    return c == a ? nullptr : c;
  }
};

// Match any P until we see T. No T = no match.
template<typename P, typename T>
struct AnyUntil {
  template<typename atom>
  static atom* match(void* ctx, Span<atom> s) {
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
  static atom* match(void* ctx, Span<atom> s) {
    auto c = AnyUnless<P,T>::match(ctx, a, b);
    return c == a ? s.fail() : {c, a};
  }
};

// Match any P unless it would match T.
template<typename P, typename T>
struct AnyUnless {
  template<typename atom>
  static atom* match(void* ctx, Span<atom> s) {
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

// Span<atom> my_special_matcher(void* ctx, Span<atom> s);
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

template <typename atom, Span<atom> (*F)(void* ctx, Span<atom> s)>
struct Ref<F> {
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    return F(ctx, s);
  }
};

template <typename T, typename atom, Span<atom> (T::*F)(Span<atom> s)>
struct Ref<F> {
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    return (((T*)ctx)->*F)(s);
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

  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    auto c = P::match(ctx, s);
    if (!c.is_valid()) {
      ref = Span<atom>();
      return c;
    }
    ref = {s.a, c.a};
    return c;
  }
};

template <StringParam name, typename atom, typename P>
struct MatchBackref {
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    auto ref = StoreBackref<name, atom, P>::ref;
    if (!ref.is_valid() || ref.is_empty()) return s.fail();

    for (size_t i = 0; i < ref.len(); i++) {
      if (s.is_empty()) return s.fail();
      if (atom_cmp(ctx, s.a, ref.a[i]) != 0) return s.fail();
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
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
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

  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
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
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
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
  static SearchResult search(void* ctx, Span<atom> s) {
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
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());

    for (auto i = 0; i < chars.str_len; i++) {
      if (s.a[0] == chars.str_val[i]) {
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
  static Span<atom> match(void* ctx, Span<atom> s) {
    if (P::match_key(a, b)) {
      parser_rewind(ctx, s);
      return P::match(ctx, a, b);
    }
    else {
      parser_rewind(ctx, s);
      return Map<rest...>::match(ctx, a, b);
    }
  }
};

template <typename P>
struct Map<P> {
  template<typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    return P::match(ctx, a, b);
  }
};

template <typename K, typename V>
struct KeyVal {
  template<typename atom>
  static Span<atom> match_key(void* ctx, Span<atom> s) {
    return K::match(ctx, a, b);
  }

  template<typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
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
  template <typename atom>
  static Span<atom> match(void* ctx, Span<atom> s) {
    return T::pattern::match(ctx, s);
  }
};

//------------------------------------------------------------------------------

};  // namespace matcheroni
