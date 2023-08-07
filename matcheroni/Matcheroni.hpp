//------------------------------------------------------------------------------
// Matcheroni is a single-header, zero-dependency toolkit that makes building
// custom pattern matchers, lexers, and parsers easier. Matcheroni is based on
// "Parsing Expression Grammars" (PEGs), which are similar in concept to
// regular expressions but behave slightly differently.

// See https://en.wikipedia.org/wiki/Parsing_expression_grammar for details.

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#define matcheroni_assert(A)

namespace matcheroni {

//------------------------------------------------------------------------------
// Matcheroni operates on spans of atoms, usually (but not always) characters.
// Valid spans have non-null begin and end pointers, empty spans have equal
// non-null begin and end pointers.

template <typename atom>
struct Span {
  using AtomType = atom;

  constexpr Span() : begin(nullptr), end(nullptr) {}
  constexpr Span(const atom* begin, const atom* end) : begin(begin), end(end) {}

  int len() const {
    matcheroni_assert(is_valid());
    return end - begin;
  }

  //----------------------------------------

  bool operator==(const Span& c) const {
    return begin == c.begin && end == c.end;
  }

  bool is_empty() const { return begin && begin == end; }
  bool is_valid() const { return begin; }

  // this must be the same as is_valid() otherwise
  // if (auto tail = match()) {}
  // fails if tail is EOF
  operator bool() const { return begin; }

  // Returns a "fail" span (nullptr, location) indicating the failure point of
  // a match.
  [[nodiscard]] Span fail() const {
    return begin ? Span(nullptr, begin) : Span(begin, end);
  }

  // Returns a span with span.begin advanced by 'offset' atoms.
  [[nodiscard]] Span advance(int offset) const {
    matcheroni_assert(begin);
    return {begin + offset, end};
  }

  //----------------------------------------

  const atom* begin;
  const atom* end;
};

//------------------------------------------------------------------------------
// Matchers will often need to compare spans against null-delimited strings ala
// strcmp(), so we provide this function for convenience.

inline int strcmp_span(const Span<char>& s2, const char* lit) {
  Span<char> s = s2;
  while (1) {
    auto ca = s.begin == s.end ? 0 : *s.begin;
    auto cb = *lit;
    if (ca != cb || ca == 0) return ca - cb;
    s.begin++;
    lit++;
  }
}

inline int strcmp_span(const Span<char>& a, const Span<char>& b) {
  if (int c = a.len() - b.len()) return c;
  for (int i = 0; i < a.len(); i++) {
    if (auto c = a.begin[i] - b.begin[i]) return c;
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
using matcher_function = Span<atom> (*)(context& ctx, Span<atom> body);

//------------------------------------------------------------------------------
// Matchers require a context object to perform two essential functions -
// compare atoms and rewind any internal state when a partial match fails.

// Since we will be matching text 99% of the time, this context class provides
// the minimal amount of code needed to run and debug Matcheroni patterns.

struct TextMatchContext {

  // We cast to unsigned char as our ranges are generally going to be unsigned.
  static int atom_cmp(char a, int b) { return (unsigned char)a - b; }

  // Checkpoint/Rewind does nothing as it doesn't interact with trace_depth.
  void* checkpoint() { return nullptr; }
  void rewind(void* bookmark) {}

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
// TextSpan tail = pattern::match(ctx, text);

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
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (body.is_empty()) return body.fail();

    if (ctx.atom_cmp(*body.begin, C) == 0) {
      return body.advance(1);
    } else {
      return Atom<rest...>::match(ctx, body);
    }
  }
};

template <auto C>
struct Atom<C> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (body.is_empty()) return body.fail();

    if (ctx.atom_cmp(*body.begin, C) == 0) {
      return body.advance(1);
    } else {
      return body.fail();
    }
  }
};

//------------------------------------------------------------------------------
// 'NotAtom' matches any atom that is _not_ in its argument list, which is a
// bit faster than using Seq<Not<Atom<...>>, AnyAtom>

template <auto C, auto... rest>
struct NotAtom {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (body.is_empty()) return body.fail();

    if (ctx.atom_cmp(*body.begin, C) == 0) {
      return body.fail();
    }
    return NotAtom<rest...>::match(ctx, body);
  }
};

template <auto C>
struct NotAtom<C> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (body.is_empty()) return body.fail();

    if (ctx.atom_cmp(*body.begin, C) == 0) {
      return body.fail();
    } else {
      return body.advance(1);
    }
  }
};

//------------------------------------------------------------------------------
// AnyAtom is equivalent to '.' in regex.

struct AnyAtom {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (body.is_empty()) return body.fail();
    return body.advance(1);
  }
};

//------------------------------------------------------------------------------
// 'Range' matches ranges of atoms, equivalent to '[a-b]' in regex.

template <auto RA, decltype(RA) RB, auto... rest>
struct Range {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (body.is_empty()) return body.fail();

    if ((ctx.atom_cmp(*body.begin, RA) >= 0) && (ctx.atom_cmp(*body.begin, RB) <= 0)) {
      return body.advance(1);
    }
    return Range<rest...>::match(ctx, body);
  }
};

template <auto RA, decltype(RA) RB>
struct Range<RA, RB> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (body.is_empty()) return body.fail();

    if ((ctx.atom_cmp(*body.begin, RA) >= 0) && (ctx.atom_cmp(*body.begin, RB) <= 0)) {
      return body.advance(1);
    }
    return body.fail();
  }
};

//------------------------------------------------------------------------------
// 'NotRange' matches ranges of atoms not in the given range, equivalent to
// '[^a-b]' in regex.

template <auto RA, decltype(RA) RB, auto... rest>
struct NotRange {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (body.is_empty()) return body.fail();

    if ((ctx.atom_cmp(*body.begin, RA) >= 0) && (ctx.atom_cmp(*body.begin, RB) <= 0)) {
      return body.fail();
    }

    return NotRange<rest...>::match(ctx, body);
  }
};

template <auto RA, decltype(RA) RB>
struct NotRange<RA, RB> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (body.is_empty()) return body.fail();

    if ((ctx.atom_cmp(*body.begin, RA) >= 0) && (ctx.atom_cmp(*body.begin, RB) <= 0)) {
      return body.fail();
    }

    return body.advance(1);
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
inline SpanType match_lit(Context& ctx, SpanType body, const char* lit, int len) {
  matcheroni_assert(body.is_valid());
  if (len > body.len()) return body.fail();

  for (int i = 0; i < len; i++) {
    if (ctx.atom_cmp(*body.begin, *lit)) return body.fail();
    body = body.advance(1);
    lit++;
  }
  return body;
}

template <StringParam lit>
struct Lit {
  template <typename Context, typename SpanType>
  static SpanType match(Context& ctx, SpanType body) {
    return match_lit(ctx, body, lit.str_val, lit.str_len);
  }
};

// FIXME variadic?


//------------------------------------------------------------------------------
// 'Seq' (sequence) succeeds if all of its sub-matchers succeed in order.

// Examples:
// Seq<Atom<'a'>, Atom<'b'>::match("abcd") == "cd"

template <typename P, typename... rest>
struct Seq {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    auto tail = P::match(ctx, body);
    return tail ? Seq<rest...>::match(ctx, tail) : tail;
  }
};

template <typename P>
struct Seq<P> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    return P::match(ctx, body);
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
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());

    auto bookmark = ctx.checkpoint();

    auto tail1 = P::match(ctx, body);
    if (tail1.is_valid()) {
      return tail1;
    }

    if (bookmark != ctx.checkpoint()) ctx.rewind(bookmark);
    auto tail2 = Oneof<rest...>::match(ctx, body);

    if (tail2.is_valid()) {
      return tail2;
    } else {
      // Both attempts failed, return whichever match got farther.
      return tail1.end > tail2.end ? tail1 : tail2;
    }
  }
};

template <typename P>
struct Oneof<P> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    return P::match(ctx, body);
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
  static Span<atom> match(context& ctx, Span<atom> body) {
    return P::match(ctx, body);
  }
};

//------------------------------------------------------------------------------
// 'Opt' matches 'optional' patterns, equivalent to '?' in regex.

// Opt<Atom<'a'>>::match("abcd") == "bcd"
// Opt<Atom<'a'>>::match("bcde") == "bcde"

template <typename... rest>
struct Opt {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    auto bookmark = ctx.checkpoint();
    if (auto tail = Oneof<rest...>::match(ctx, body)) return tail;
    if (bookmark != ctx.checkpoint()) ctx.rewind(bookmark);
    return body;
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
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (body.is_empty()) return body;

    while (1) {
      auto bookmark = ctx.checkpoint();
      auto tail = Oneof<rest...>::match(ctx, body);
      if (!tail.is_valid()) {
        if (bookmark != ctx.checkpoint()) ctx.rewind(bookmark);
        break;
      }
      body = tail;
    }

    return body;
  }
};

//------------------------------------------------------------------------------
// Nothing always succeeds in matching nothing. Makes a good placeholder. :)

struct Nothing {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    return body;
  }
};

//------------------------------------------------------------------------------
// 'Some' matches one or more copies of a pattern, equivalent to '+' in regex.

// Some<Atom<'a'>>::match("aaaab") == "b"
// Some<Atom<'a'>>::match("bbbbc") == nullptr

template <typename... rest>
struct Some {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    auto tail = Any<rest...>::match(ctx, body);
    return (tail == body) ? body.fail() : tail;
  }
};

//------------------------------------------------------------------------------
// The 'And' predicate matches a pattern but does _not_ advance the cursor.
// Used for lookahead.

// FIXME variadic?

// And<Atom<'a'>>::match("abcd") == "abcd"
// And<Atom<'a'>>::match("bcde") == nullptr

template <typename P>
struct And {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    auto bookmark = ctx.checkpoint();
    auto tail = P::match(ctx, body);
    if (bookmark != ctx.checkpoint()) ctx.rewind(bookmark);
    return tail.is_valid() ? body : tail;
  }
};

//------------------------------------------------------------------------------
// The 'Not' predicate is the logical negation of the 'And' predicate.

// Not<Atom<'a'>>::match("abcd") == nullptr
// Not<Atom<'a'>>::match("bcde") == "bcde"

// FIXME variadic?

template <typename P>
struct Not {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    auto bookmark = ctx.checkpoint();
    auto tail = P::match(ctx, body);
    if (bookmark != ctx.checkpoint()) ctx.rewind(bookmark);
    return tail.is_valid() ? body.fail() : body;
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
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (auto tail = P::match(ctx, body)) body = tail;
    return SeqOpt<rest...>::match(ctx, body);
  }
};

template <typename P>
struct SeqOpt<P> {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (auto tail = P::match(ctx, body)) body = tail;
    return body;
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
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    auto tail = Seq<rest...>::match(ctx, body);
    return tail == body ? body.fail() : tail;
  }
};

//------------------------------------------------------------------------------
// 'Rep' is equivalent to '{N}' in regex.

template <int N, typename P>
struct Rep {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    for (auto i = 0; i < N; i++) {
      body = P::match(ctx, body);
      if (!body.is_valid()) break;
    }
    return body;
  }
};

//------------------------------------------------------------------------------
// 'RepRange' is equivalent '{M,N}' in regex. C++ won't let us overload Rep<>
// to handle both {N} and {M,N} :( .

template <int M, int N, typename P>
struct RepRange {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    for (auto i = 0; i < N; i++) {
      auto tail = P::match(ctx, body);
      if (!tail.is_valid()) {
        if (i < M) return tail;
        else break;
      }
      body = tail;
    }
    return body;
  }
};

//------------------------------------------------------------------------------
// 'Until' matches anything until we see the given pattern or we hit EOF.
// The pattern is _not_ consumed.

// Equivalent to Any<Seq<Not<M>,AnyAtom>>


template<typename P>
struct Until {
  template<typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    while(1) {
      if (body.is_empty()) return body;
      auto bookmark = ctx.checkpoint();
      auto tail = P::match(ctx, body);
      if (tail.is_valid()) {
        if (bookmark != ctx.checkpoint()) ctx.rewind(bookmark);
        return body;
      }
      body = body.advance(1);
    }
  }
};

#if 0
// These patterns need better names and explanations...

// Match some P until we see T. No T = no match.
template<typename P, typename T>
struct SomeUntil {
  template<typename atom>
  static atom* match(context& ctx, Span<atom> body) {
    auto tail = AnyUntil<P,T>::match(ctx, body);
    return tail == body ? body.fail() : tail;
  }
};

// Match any P until we see T. No T = no match.
template<typename P, typename T>
struct AnyUntil {
  template<typename atom>
  static atom* match(context& ctx, Span<atom> body) {
    while(!body.is_empty()) {
      if (T::match(ctx, body)) return a;
      if (auto tail = P::match(ctx, body)) {
        body = tail;
      }
      else {
        return body.fail();
      }
    }
    return body.fail();
  }
};

// Match some P unless it would match T.
template<typename P, typename T>
struct SomeUnless {
  template<typename atom>
  static atom* match(context& ctx, Span<atom> body) {
    auto tail = AnyUnless<P,T>::match(ctx, body);
    return tail == a ? body.fail() : tail;
  }
};

// Match any P unless it would match T.
template<typename P, typename T>
struct AnyUnless {
  template<typename atom>
  static atom* match(context& ctx, Span<atom> body) {
    while(!body.is_empty()) {
      if (T::match(ctx, body)) return body;
      if (auto tail = P::match(ctx, a, b)) {
        body = tail;
      }
      else {
        break;
      }
    }
    return body;
  }
};
#endif

//------------------------------------------------------------------------------
// 'Ref' is used to call a user-defined matcher function from a Matcheroni
// pattern.

// 'Ref' can also be used to call member functions. You _MUST_ pass a pointer
// to an object via the 'ctx' parameter when using these.

// Span<atom> my_special_matcher(context& ctx, Span<atom> body);
// using pattern = Ref<my_special_matcher>;

// struct Foo {
//   Span<atom> match(Span<atom> body);
// };
//
// using pattern = Ref<&Foo::match>;
// Foo my_foo;
// auto tail = pattern::match(&my_foo, body);

template <auto F>
struct Ref;

template <typename context, typename atom, Span<atom> (*func)(context& ctx, Span<atom> body)>
struct Ref<func> {
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    return func(ctx, body);
  }
};

template <typename context, typename atom, Span<atom> (context::*func)(Span<atom> body)>
struct Ref<func> {
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    return (ctx.*func)(body);
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
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    auto tail = P::match(ctx, body);
    if (!tail.is_valid()) {
      ref = body.fail();
      return tail;
    }
    ref = {body.begin, tail.begin};

    //printf("Backref: `");
    //for (auto d = body.a; d < c.a; d++) putc(*d, stdout);
    //printf("`\n");
    return tail;
  }
};

template <StringParam name, typename atom, typename P>
struct MatchBackref {

  template<typename context>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());

    auto ref = StoreBackref<name, atom, P>::ref;
    if (!ref.is_valid()) return body.fail();

    for (int i = 0; i < ref.len(); i++) {
      if (body.is_empty()) return body.fail();
      if (ctx.atom_cmp(*body.begin, ref.begin[i])) return body.fail();
      body = body.advance(1);
    }

    return body;
  }
};

//------------------------------------------------------------------------------
// 'DelimitedBlock' is equivalent to Seq<ldelim, Any<body>, rdelim>, but it
// tries to match rdelim before body which can save matching time.

template <typename ldelim, typename element, typename rdelim>
struct DelimitedBlock {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());

    body = ldelim::match(ctx, body);
    if (!body.is_valid()) return body;

    while (1) {
      auto tail = rdelim::match(ctx, body);
      if (tail.is_valid()) return tail;
      body = element::match(ctx, body);
      if (!body.is_valid()) break;
    }
    return body;
  }
};

//------------------------------------------------------------------------------
// 'DelimitedList' is the same as 'DelimitedBlock' except that it adds a
// separator pattern between items.

template <typename ldelim, typename item, typename separator, typename rdelim>
struct DelimitedList {
  // Might be faster to do this in terms of comma_separated, etc?

  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());

    body = ldelim::match(ctx, body);
    if (!body.is_valid()) return body;

    while (1) {
      auto tail1 = rdelim::match(ctx, body);
      if (tail1.is_valid()) return tail1;
      body = item::match(ctx, body);
      if (!body.is_valid()) break;

      auto tail2 = rdelim::match(ctx, body);
      if (tail2.is_valid()) return tail2;
      body = separator::match(ctx, body);
      if (!body.is_valid()) break;
    }
    return body;
  }
};

//------------------------------------------------------------------------------
// 'EOL' matches newline and EOF, but does not advance past it.

struct EOL {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());
    if (body.is_empty()) return body;
    if (body.begin[0] == atom('\n')) return body;
    return body.fail();
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
  static SearchResult search(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());

    auto cursor = body;
    while(1) {
      auto tail = P::match(ctx, cursor, b);
      if (tail) {
        return {cursor-a, tail-cursor};
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
  static Span<atom> match(context& ctx, Span<atom> body) {
    matcheroni_assert(body.is_valid());

    for (auto i = 0; i < chars.str_len; i++) {
      if (ctx.atom_cmp(body.begin[0], chars.str_val[i]) == 0) {
        return body.advance(1);
      }
    }
    return body.fail();
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
  static Span<atom> match(context& ctx, Span<atom> body) {
    auto bookmark = ctx.checkpoint();
    if (P::match_key(a, b)) {
      if (bookmark != ctx.checkpoint()) ctx.rewind(bookmark);
      return P::match(ctx, a, b);
    }
    else {
      if (bookmark != ctx.checkpoint()) ctx.rewind(bookmark);
      return Map<rest...>::match(ctx, a, b);
    }
  }
};

template <typename P>
struct Map<P> {
  template<typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    return P::match(ctx, a, b);
  }
};

template <typename K, typename V>
struct KeyVal {
  template<typename atom>
  static Span<atom> match_key(context& ctx, Span<atom> body) {
    return K::match(ctx, a, b);
  }

  template<typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
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
// auto tail = Some<MyPattern>::match(ctx, a, b);

template <typename T>
struct PatternWrapper {
  template <typename context, typename atom>
  static Span<atom> match(context& ctx, Span<atom> body) {
    return T::pattern::match(ctx, body);
  }
};

//------------------------------------------------------------------------------

};  // namespace matcheroni
