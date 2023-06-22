#ifndef __MATCHERONI_H__
#define __MATCHERONI_H__

#include <compare>

#ifdef MATCHERONI_USE_NAMESPACE
namespace matcheroni {
#endif

//------------------------------------------------------------------------------
// Matcheroni is based on building trees of "matcher" functions. A matcher takes
// a range of "atoms" (could be characters, could be some application-specific
// type) as input and returns either the endpoint of the match if found, or
// nullptr if a match was not found.
// Matchers must always handle null pointers and empty ranges.

template<typename atom>
using matcher_function = atom* (*) (atom* a, atom* b);

//------------------------------------------------------------------------------
// Matcheroni needs some way to compare different types of atoms - for
// convenience, comparators for "const char" are provided here.
// Comparators should return <0 for a<b, ==0 for a==b, and >0 for a>b.

// enum class _Ord : type { equivalent = 0, less = -1, greater = 1 };

inline int atom_cmp(const char& a, const char& b) {
  return int(a) - int(b);
}

//------------------------------------------------------------------------------
// Matchers will often need to compare literal strings of text, so some helpers
// are provided here as well.

inline const char* match_text(const char* text, const char* a, const char* b) {
  auto c = a;
  for (;c < b && (*c == *text) && *text; c++, text++);
  if (*text) return nullptr;
  return c;
}

inline const char* match_text(const char** texts, int text_count, const char* a, const char* b) {
  for (auto i = 0; i < text_count; i++) {
    if (auto t = match_text(texts[i], a, b)) {
      return t;
    }
  }
  return nullptr;
}

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
  static atom* match(atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(*a, C) == 0) {
      return a + 1;
    } else {
      return Atom<rest...>::match(a, b);
    }
  }
};

template <auto C>
struct Atom<C> {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(*a, C) == 0) {
      return a + 1;
    } else {
      return nullptr;
    }
  }
};

struct AnyAtom {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    return a + 1;
  }
};

//------------------------------------------------------------------------------
// 'Seq' (sequence) succeeds if all of its sub-matchers succeed in order.

// Examples:
// Seq<Atom<'a'>, Atom<'b'>::match("abcd") == "cd"

template<typename P, typename... rest>
struct Seq {

  template<typename atom>
  static atom* match(atom* a, atom* b) {
    auto c = P::match(a, b);
    return c ? Seq<rest...>::match(c, b) : nullptr;
  }
};

template<typename P>
struct Seq<P> {

  template<typename atom>
  static atom* match(atom* a, atom* b) {
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
  static atom* match(atom* a, atom* b) {
    auto c = P::match(a, b);
    return c ? c : Oneof<rest...>::match(a, b);
  }
};

template <typename P>
struct Oneof<P> {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    return P::match(a, b);
  }
};

//------------------------------------------------------------------------------
// Zero-or-one 'optional' patterns, equivalent to M? in regex.

// Opt<Atom<'a'>>::match("abcd") == "bcd"
// Opt<Atom<'a'>>::match("bcde") == "bcde"

template<typename P>
struct Opt {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    auto c = P::match(a, b);
    return c ? c : a;
  }
};

//------------------------------------------------------------------------------
// Zero-or-more patterns, roughly equivalent to M* in regex.
// HOWEVER - Seq<Any<Atom<'a'>>, Atom<'a'>> (unlike "a*a" in regex) will
// _never_ match anything, as the first Any<> is greedy and consumes all 'a's
// without doing any backtracking.

// Any<Atom<'a'>>::match("aaaab") == "b"
// Any<Atom<'a'>>::match("bbbbc") == "bbbbc"

template<typename P>
struct Any {

  template<typename atom>
  static atom* match(atom* a, atom* b) {
    while(auto c = P::match(a, b)) {
      a = c;
    }
    return a;
  }
};

//------------------------------------------------------------------------------
// One-or-more patterns, equivalent to M+ in regex.

// Some<Atom<'a'>>::match("aaaab") == "b"
// Some<Atom<'a'>>::match("bbbbc") == nullptr

template<typename P>
struct Some {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    auto c = P::match(a, b);
    return c ? Any<P>::match(c, b) : nullptr;
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
  static atom* match(atom* a, atom* b) {
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
  static atom* match(atom* a, atom* b) {
    auto c = P::match(a, b);
    return c ? nullptr : a;
  }
};

//------------------------------------------------------------------------------
// Equivalent to Some<OneOf<>>
// FIXME should we leave this in?

template <typename P, typename... rest>
struct SomeOf {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    auto c = match1(a, b);
    if (c == nullptr) return nullptr;
    while (auto end = match1(c, b)) {
      c = end;
    }
    return c;
  }

  template<typename atom>
  static atom* match1(atom* a, atom* b) {
    auto c = P::match(a, b);
    return c ? c : Oneof<rest...>::match(a, b);
  }
};

template <typename P>
struct SomeOf<P> {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    return P::match(a, b);
  }

  template<typename atom>
  static atom* match1(atom* a, atom* b) {
    return P::match(a, b);
  }
};

//------------------------------------------------------------------------------
// Repetition, equivalent to M{N} in regex.

template<int N, typename P>
struct Rep {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    for(auto i = 0; i < N; i++) {
      auto c = P::match(a, b);
      if (!c) return nullptr;
      a = c;
    }
    return a;
  }
};

//------------------------------------------------------------------------------
// Atom-not-in-set matcher, which is a bit faster than using
// Seq<Not<Atom<...>>, AnyAtom>

template <auto C, auto... rest>
struct NotAtom {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(*a, C) == 0) return nullptr;
    return NotAtom<rest...>::match(a, b);
  }
};

template <auto C>
struct NotAtom<C> {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    return atom_cmp(*a, C) == 0 ? nullptr : a + 1;
  }
};

//------------------------------------------------------------------------------
// Ranges of atoms, inclusive.

template<auto RA, decltype(RA) RB>
struct Range {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(*a, RA) < 0) return nullptr;
    if (atom_cmp(*a, RB) > 0) return nullptr;
    return a + 1;
  }
};

//------------------------------------------------------------------------------
// Advances the cursor until the pattern matches or we hit EOF. Does _not_
// consume the pattern. Equivalent to Any<Seq<Not<M>,AnyAtom>>

template<typename P>
struct Until {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    while(a < b) {
      if (P::match(a, b)) return a;
      a++;
    }
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// References to bare matcher functions.

// const char* my_special_matcher(const char* a, const char* b);
// using pattern = Ref<my_special_matcher>;

template<auto& M>
struct Ref {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    return M(a, b);
  }
};

//------------------------------------------------------------------------------
// Stores and matches backreferences, used for raw string delimiters in the C
// lexer. Note that the backreference is stored as a static pointer in the
// StoreBackref template, so be careful of nesting as you could clobber it.

template<typename P>
struct StoreBackref {

  inline static const void* start;
  inline static int size;

  template<typename atom>
  static atom* match(atom* a, atom* b) {
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
  static atom* match(atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    atom* start = (atom*)(StoreBackref<P>::start);
    int size = StoreBackref<P>::size;
    if (a + size > b) return nullptr;
    for (int i = 0; i < size; i++) {
      if(atom_cmp(a[i], start[i]) != 0) return nullptr;
    }
    return a + size;
  }
};

//------------------------------------------------------------------------------
// Matches newline and EOF, but does not advance past it.

struct EOL {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
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
  static atom* match(atom* a, atom* b) {
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
// You'll need to define atom_cmp(atom& a, StringParam<N>& b) to use this.

template<StringParam lit>
struct Keyword {

  template<typename atom>
  static atom* match(atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(*a, lit) == 0) {
      return a + 1;
    } else {
      return nullptr;
    }
  }
};

//------------------------------------------------------------------------------
// Matches larger sets of atoms packed into a string literal.

// Charset<"abcdef">::match("defg") == "efg"

template<StringParam chars>
struct Charset {

  template<typename atom>
  static atom* match(atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    for (auto i = 0; i < sizeof(chars.value); i++) {
      if (*a == chars.value[i]) {
        return a + 1;
      }
    }
    return nullptr;
  }

};

//------------------------------------------------------------------------------
// 'Map' returns the result of the first matcher whose 'match_key' matches the
// input. Unlike 'Oneof', if the key pattern matches but the value pattern does
// not, the rest of the options will _not_ be checked. This can make matching
// much faster by allowing large arrays of options to be broken down into
// skippable groups.

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

template<typename P, typename... rest>
struct Map {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    if (P::match_key(a, b)) {
      return P::match(a, b);
    }
    else {
      return Map<rest...>::match(a, b);
    }
  }
};

template <typename P>
struct Map<P> {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    return P::match(a, b);
  }
};

template <typename K, typename V>
struct KeyVal {
  template<typename atom>
  static atom* match_key(atom* a, atom* b) {
    return K::match(a, b);
  }

  template<typename atom>
  static atom* match(atom* a, atom* b) {
    return V::match(a, b);
  }
};

//------------------------------------------------------------------------------

template<typename NT>
struct PatternWrapper {
  template<typename atom>
  static atom* match(atom* a, atom* b) {
    return NT::pattern::match(a, b);
  }
};

//------------------------------------------------------------------------------

#ifdef MATCHERONI_USE_NAMESPACE
}; // namespace Matcheroni
#endif

#endif // #ifndef __MATCHERONI_H__
