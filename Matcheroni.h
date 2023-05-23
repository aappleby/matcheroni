#ifndef __MATCHERONI_H__
#define __MATCHERONI_H__

namespace matcheroni {

//------------------------------------------------------------------------------
// Matcheroni is based on building trees of "matcher" functions. A matcher takes
// a range of atoms as input and returns either the endpoint of the match if
// found, or nullptr if a match was not found.
// Matchers must always handle null pointers and empty ranges.

template<typename atom>
using matcher = const atom* (*) (const atom* a, const atom* b, void* ctx);

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
  using atom = decltype(C);

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    if (!a || a == b) return nullptr;
    if (*a == C) {
      return a + 1;
    } else {
      return Atom<rest...>::match(a, b, ctx);;
    }
  }
};

template <auto C>
struct Atom<C> {
  using atom = decltype(C);

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    if (!a || a == b) return nullptr;
    if (*a == C) {
      return a + 1;
    } else {
      return nullptr;
    }
  }
};

template<typename _atom>
struct AnyAtom {
  using atom = _atom;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    if (!a || a == b) return nullptr;
    return a + 1;
  }
};

//------------------------------------------------------------------------------
// The 'Seq' matcher succeeds if all of its sub-matchers succeed in order.

// Examples:
// Seq<Atom<'a'>, Atom<'b'>::match("abcd") == "cd"

template<typename P, typename... rest>
struct Seq {
  using atom = P::atom;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    auto c = P::match(a, b, ctx);
    return c ? Seq<rest...>::match(c, b, ctx) : nullptr;
  }
};

template<typename P>
struct Seq<P> {
  using atom = P::atom;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    return P::match(a, b, ctx);
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
  using atom = P::atom;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    auto c = P::match(a, b, ctx);
    return c ? c : Oneof<rest...>::match(a, b, ctx);
  }
};


template <typename P>
struct Oneof<P> {
  using atom = P::atom;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    return P::match(a, b, ctx);
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
  using atom = P::atom;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    while(auto c = P::match(a, b, ctx)) a = c;
    return a;
  }
};

//------------------------------------------------------------------------------
// Zero-or-one 'optional' patterns, equivalent to M? in regex.

// Opt<Atom<'a'>>::match("abcd") == "bcd"
// Opt<Atom<'a'>>::match("bcde") == "bcde"

template<typename P>
struct Opt {
  using atom = P::atom;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    auto c = P::match(a, b, ctx);
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
  using atom = P::atom;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    auto c = P::match(a, b, ctx);
    return c ? Any<P>::match(c, b, ctx) : nullptr;
  }
};

//------------------------------------------------------------------------------
// Repetition, equivalent to M{N} in regex.

template<int N, typename P>
struct Rep {
  using atom = P::atom;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    for(auto i = 0; i < N; i++) {
      auto c = P::match(a, b, ctx);
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
  using atom = P::atom;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    auto c = P::match(a, b, ctx);
    return c ? a : nullptr;
  }
};

//------------------------------------------------------------------------------
// The 'not' predicate, the logical negation of the 'and' predicate.

// Not<Atom<'a'>>::match("abcd") == nullptr
// Not<Atom<'a'>>::match("bcde") == "bcde"

template<typename P>
struct Not {
  using atom = P::atom;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    auto c = P::match(a, b, ctx);
    return c ? nullptr : a;
  }
};

//------------------------------------------------------------------------------
// Matches EOF, but does not advance past it.

template <typename C>
struct EOF {
  using atom = C;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    return a == b ? a : nullptr;
  }
};

//------------------------------------------------------------------------------
// Atom-not-in-set matcher, which is a bit faster than using
// Seq<Not<Atom<...>>, AnyAtom>

template <auto C, auto... rest>
struct NotAtom {
  using atom = decltype(C);

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    if (!a || a == b) return nullptr;
    if (*a == C) return nullptr;
    return NotAtom<rest...>::match(a, b, ctx);
  }
};

template <auto C>
struct NotAtom<C> {
  using atom = decltype(C);

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    if (!a || a == b) return nullptr;
    return (*a == C) ? nullptr : a + 1;
  }
};

//------------------------------------------------------------------------------
// Ranges of atoms, inclusive. Equivalent to [a-z] in regex.

template<auto RA, decltype(RA) RB>
struct Range {
  using atom = decltype(RA);

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    if (!a || a == b) return nullptr;
    if (*a >= RA && *a <= RB) {
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
  using atom = P::atom;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    while(a < b) {
      if (P::match(a, b, ctx)) return a;
      a++;
    }
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// References to other matchers. Enables recursive matchers.

template<auto& M>
struct Ref {
  template<typename atom>
  static const atom* match(const atom* a, const atom* b, void* ctx) {
    return M(a, b, ctx);
  }
};

//------------------------------------------------------------------------------
// To use backreferences, a matcher above these ones must pass a pointer to a
// Backref object in the ctx parameter.

template<typename atom>
struct Backref {
  const atom* start;
  int size;
};

template<typename P>
struct StoreBackref {
  using atom = P::atom;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    auto c = P::match(a, b, ctx);
    auto ref = (Backref<atom>*)ctx;
    if (!c || !ref) return nullptr;

    ref->start = a;
    ref->size = int(c - a);
    return c;
  }

};

template<typename _atom>
struct MatchBackref {
  using atom = _atom;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    if (!a || a == b) return nullptr;
    auto ref = (Backref<atom>*)ctx;
    if (!ref) return nullptr;
    if (a + ref->size > b) return nullptr;
    for (int i = 0; i < ref->size; i++) {
      if(a[i] != ref->start[i]) return nullptr;
    }
    return a + ref->size;
  }
};



































//------------------------------------------------------------------------------
// Matches newline and EOF, but does not advance past it.

struct EOL {
  using atom = char;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    if (!a) return nullptr;
    if (a == b) return a;
    if (*a == atom('\n')) return a;
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// Not a matcher, but a template helper that allows us to use strings as
// template arguments. The parameter behaves as a fixed-length character array
// that does ___NOT___ include the trailing null.

template<int N>
struct StringParam {
  constexpr StringParam(const char (&str)[N]) {
    for (int i = 0; i < N-1; i++) value[i] = str[i];
  }
  char value[N-1];
};

//------------------------------------------------------------------------------
// Matches string literals. Does ___NOT___ include the trailing null.

// Lit<"foo">::match("foobar") == "bar"

template<StringParam lit>
struct Lit {
  using atom = char;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    if (!a || a == b) return nullptr;
    if (a + sizeof(lit.value) > b) return nullptr;
    for (auto i = 0; i < sizeof(lit.value); i++) {
      if (a[i] != lit.value[i]) return nullptr;
    }
    return a + sizeof(lit.value);
  }
};

//------------------------------------------------------------------------------
// 'OneofLit' returns the first match in an array of literals.

/*
template <StringParam M1, StringParam... rest>
struct OneofLit {
  static const char* match(const char* cursor, void* ctx) {
    if (auto end = Lit<M1>::match(cursor, ctx)) return end;
    return OneofLit<rest...>::match(cursor, ctx);
  }
};

template <StringParam M1>
struct OneofLit<M1> {
  static const char* match(const char* cursor, void* ctx) {
    return Lit<M1>::match(cursor, ctx);
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
  using atom = char;

  static const atom* match(const atom* a, const atom* b, void* ctx) {
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
  static const char* match(const char* cursor, void* ctx) {
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
  static const char* match(const char* cursor, void* ctx) {
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
