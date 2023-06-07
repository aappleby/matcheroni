#ifndef __MATCHERONI_H__
#define __MATCHERONI_H__

// FIXME minimal push/pop stack for matchers

namespace matcheroni {

//------------------------------------------------------------------------------
// Matcheroni is based on building trees of "matcher" functions. A matcher takes
// a range of atoms as input and returns either the endpoint of the match if
// found, or nullptr if a match was not found.
// Matchers must always handle null pointers and empty ranges.

template<typename atom>
struct StackEntry;

template<typename atom>
using matcher = const atom* (*) (const atom* a, const atom* b);

template<typename atom>
struct StackEntry {
  matcher<atom> m;
  const atom* a;
  const atom* b;
};

//------------------------------------------------------------------------------

/*
template<typename atom1, typename atom2>
inline bool atom_eq(const atom1& a, const atom2& b);

template<typename atom1, typename atom2>
inline bool atom_lte(const atom1& a, const atom2& b);

template<typename atom1, typename atom2>
inline bool atom_gte(const atom1& a, const atom2& b);
*/

inline bool atom_eq(const char& a, const char& b) {
  return a == b;
}

inline bool atom_lte(const char& a, const char& b) {
  return a <= b;
}

inline bool atom_gte(const char& a, const char& b) {
  return a >= b;
}

/*
template<typename atom1, typename atom2>
inline bool atom_eq(const atom1& a, const atom2& b) {
  return a == b;
}

template<typename atom1, typename atom2>
inline bool atom_lte(const atom1& a, const atom2& b) {
  return a <= b;
}

template<typename atom1, typename atom2>
inline bool atom_gte(const atom1& a, const atom2& b) {
  return a >= b;
}
*/

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

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_eq(*a, C)) {
      return a + 1;
    } else {
      return Atom<rest...>::match(a, b);
    }
  }
};

template <auto C>
struct Atom<C> {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_eq(*a, C)) {
      return a + 1;
    } else {
      return nullptr;
    }
  }
};

struct AnyAtom {
  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    if (!a || a == b) return nullptr;
    return a + 1;
  }
};

//------------------------------------------------------------------------------

/*
template<typename P>
struct Push {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    auto stack = (StackEntry*)ctx;
    stack[0].m = P::match;
    stack[0].a = a;
    stack[0].b = P::match(a, b, stack + 1);
    return stack[0].b;
  }
};

template<typename P>
struct Pop {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    auto stack = (StackEntry*)ctx;
    assert(
  }
};
*/

//------------------------------------------------------------------------------
// The 'Seq' matcher succeeds if all of its sub-matchers succeed in order.

// Examples:
// Seq<Atom<'a'>, Atom<'b'>::match("abcd") == "cd"

template<typename P, typename... rest>
struct Seq {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    auto c = P::match(a, b);
    return c ? Seq<rest...>::match(c, b) : nullptr;
  }
};

template<typename P>
struct Seq<P> {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    return P::match(a, b);
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

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    auto c = P::match(a, b);
    return c ? c : Oneof<rest...>::match(a, b);
  }
};


template <typename P>
struct Oneof<P> {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    return P::match(a, b);
  }
};

//------------------------------------------------------------------------------
// Zero-or-more patterns, roughly equivalent to M* in regex.
// HOWEVER - Seq<Any<Atom<'a'>>, Atom<'a'>> (unlike "a*a" in regex) will
// _never_ match anything, as the first Any<> is greedy and consumes all 'a's
// without doing any backtracking.

// Examples:
// Any<Atom<'a'>>::match("aaaab") == "b"
// Any<Atom<'a'>>::match("bbbbc") == "bbbbc"

template<typename P>
struct Any {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    while(auto c = P::match(a, b)) a = c;
    return a;
  }
};

//------------------------------------------------------------------------------
// Zero-or-one 'optional' patterns, equivalent to M? in regex.

// Opt<Atom<'a'>>::match("abcd") == "bcd"
// Opt<Atom<'a'>>::match("bcde") == "bcde"

template<typename P>
struct Opt {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    auto c = P::match(a, b);
    return c ? c : a;
  }
};

//------------------------------------------------------------------------------
// One-or-more patterns, equivalent to M+ in regex.

// Examples:
// Some<Atom<'a'>>::match("aaaab") == "b"
// Some<Atom<'a'>>::match("bbbbc") == nullptr

template<typename P>
struct Some {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    auto c = P::match(a, b);
    return c ? Any<P>::match(c, b) : nullptr;
  }
};

//------------------------------------------------------------------------------
// Repetition, equivalent to M{N} in regex.

template<int N, typename P>
struct Rep {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    for(auto i = 0; i < N; i++) {
      auto c = P::match(a, b);
      if (!c) return nullptr;
      a = c;
    }
    return a;
  }
};

//------------------------------------------------------------------------------
// The 'and' predicate, which matches but does _not_ advance the cursor. Used
// for lookahead.

// And<Atom<'a'>>::match("abcd") == "abcd"
// And<Atom<'a'>>::match("bcde") == nullptr

template<typename P>
struct And {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    auto c = P::match(a, b);
    return c ? a : nullptr;
  }
};

//------------------------------------------------------------------------------
// The 'not' predicate, the logical negation of the 'and' predicate.

// Not<Atom<'a'>>::match("abcd") == nullptr
// Not<Atom<'a'>>::match("bcde") == "bcde"

template<typename P>
struct Not {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    auto c = P::match(a, b);
    return c ? nullptr : a;
  }
};

//------------------------------------------------------------------------------
// Matches EOF, but does not advance past it.

template <typename C>
struct EOF {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    return a == b ? a : nullptr;
  }
};

//------------------------------------------------------------------------------
// Atom-not-in-set matcher, which is a bit faster than using
// Seq<Not<Atom<...>>, AnyAtom>

template <auto C, auto... rest>
struct NotAtom {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_eq(*a, C)) return nullptr;
    return NotAtom<rest...>::match(a, b);
  }
};

template <auto C>
struct NotAtom<C> {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    if (!a || a == b) return nullptr;
    return atom_eq(*a, C) ? nullptr : a + 1;
  }
};

//------------------------------------------------------------------------------
// Ranges of atoms, inclusive. Equivalent to [a-z] in regex.

template<auto RA, decltype(RA) RB>
struct Range {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_gte(*a, RA) && atom_lte(*a, RB)) {
      return a + 1;
    }
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// Advances the cursor until the pattern matches or we hit EOF. Does _not_
// consume the pattern. Equivalent to Any<Seq<Not<M>,AnyAtom>>

template<typename P>
struct Until {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    while(a < b) {
      if (P::match(a, b)) return a;
      a++;
    }
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// References to other matchers. Enables recursive matchers.

template <typename R, typename... A> R ret(const R *(*)(A...));

template<auto& M>
struct Ref {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    return M(a, b);
  }
};

//------------------------------------------------------------------------------

template<typename P>
struct StoreBackref {

  inline static const void* start;
  inline static int size;

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    auto c = P::match(a, b);
    if (!c) return nullptr;
    start = a;
    size = int(c - a);
    return c;
  }
};

template<typename P>
struct MatchBackref {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    if (!a || a == b) return nullptr;
    const atom* start = (const atom*)(StoreBackref<P>::start);
    int size = StoreBackref<P>::size;
    if (a + size > b) return nullptr;
    for (int i = 0; i < size; i++) {
      if(!atom_eq(a[i], start[i])) return nullptr;
    }
    return a + size;
  }
};

//------------------------------------------------------------------------------



































//------------------------------------------------------------------------------
// Matches newline and EOF, but does not advance past it.

struct EOL {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    if (!a) return nullptr;
    if (a == b) return a;
    if (*a == atom('\n')) return a;
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// Not a matcher, but a template helper that allows us to use strings as
// template parameters. The parameter behaves as a fixed-length character array
// that does ___NOT___ include the trailing null.

template<int N>
struct StringParam {
  constexpr static auto len = N-1;
  constexpr StringParam(const char (&str)[N]) {
    for (int i = 0; i < len; i++) value[i] = str[i];
  }
  char value[len];
};

//------------------------------------------------------------------------------
// Matches string literals. Does ___NOT___ include the trailing null.

// Lit<"foo">::match("foobar") == "bar"

template<StringParam lit>
struct Lit {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    if (!a || a == b) return nullptr;
    if (a + sizeof(lit.value) > b) return nullptr;

    for (auto i = 0; i < lit.len; i++) {
      if (a[i] != lit.value[i]) return nullptr;
    }
    return a + sizeof(lit.value);
  }
};


//------------------------------------------------------------------------------
// Matches string literals as if they were atoms. Does ___NOT___ include the
// trailing null.

template<StringParam lit>
struct AtomLit {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_eq(*a, lit)) {
      return a + 1;
    } else {
      return nullptr;
    }
  }
};

#if 0
//------------------------------------------------------------------------------
// Not a matcher, but a template helper that allows us to use arrays of strings
// as template arguments.

template<int N>
struct StringParams {
  constexpr StringParams(const char* const (&strs)[N]) {
    for (int i = 0; i < N; i++) value[i] = strs[i];
  }
  const char* value[N];
};


//------------------------------------------------------------------------------
// Matches arrays of string literals. Does ___NOT___ include the trailing null.

// const char* keywords[] = {"foo", "bar"};
// Lits<keywords>::match("foobar") == "bar"

//template<const char* const* lits, int N>
template<StringParams lits>
struct Lits {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    if (!a || a == b) return nullptr;

    auto N = sizeof(lits.value) / sizeof(lits.value[0]);
    for (auto i = 0; i < N; i++) {
      auto lit = lits.value[i];
      auto c = a;
      for (;c < b && (*c == *lit) && *lit; c++, lit++);
      if (*lit == 0) return c;
    }

    return nullptr;
  }
};
#endif

//------------------------------------------------------------------------------
// 'OneofLit' returns the first match in an array of literals.

/*
template <StringParam M1, StringParam... rest>
struct OneofLit {
  static const char* match(const char* cursor) {
    if (auto end = Lit<M1>::match(cursor)) return end;
    return OneofLit<rest...>::match(cursor);
  }
};

template <StringParam M1>
struct OneofLit<M1> {
  static const char* match(const char* cursor) {
    return Lit<M1>::match(cursor);
  }
};
*/

//------------------------------------------------------------------------------
// Matches larger sets of chars, digraphs, and trigraphs packed into a string
// literal.

// Charset<"abcdef">::match("defg") == "efg"
// Digraphs<"aabbcc">::match("bbccdd") == "ccddee"
// Trigraphs<"abc123xyz">::match("123456") == "456"

template<StringParam chars>
struct Charset {

  template<typename atom>
  static const atom* match(const atom* a, const atom* b) {
    if (!a || a == b) return nullptr;
    for (auto i = 0; i < sizeof(chars.value); i++) {
      if (*a == chars.value[i]) {
        return a + 1;
      }
    }
    return nullptr;
  }

};

/*
template<StringParam chars>
struct Digraphs {
  static const char* match(const char* cursor) {
    if(!cursor) return nullptr;
    for (auto i = 0; i < sizeof(chars.value); i += 2) {
      if (cursor[0] == chars.value[i+0] &&
          cursor[1] == chars.value[i+1]) {
        return cursor + 2;
      }
    }
    return nullptr;
  }
};

template<StringParam chars>
struct Trigraphs {
  static const char* match(const char* cursor) {
    if(!cursor) return nullptr;
    for (auto i = 0; i < sizeof(chars.value); i += 3) {
      if (cursor[0] == chars.value[i+0] &&
          cursor[1] == chars.value[i+1] &&
          cursor[2] == chars.value[i+2]) {
        return cursor + 3;
      }
    }
    return nullptr;
  }
};
*/

//------------------------------------------------------------------------------

}; // namespace Matcheroni

#endif // #ifndef __MATCHERONI_H__
