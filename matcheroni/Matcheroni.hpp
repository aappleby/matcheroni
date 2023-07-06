#ifndef __MATCHERONI_H__
#define __MATCHERONI_H__

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
using matcher_function = atom* (*) (void* ctx, atom* a, atom* b);

//------------------------------------------------------------------------------
// Matchers will often need to compare literal strings of text, so some helpers
// are provided here as well.

inline int cmp_span_lit(const char* aa, const char* ab, const char* b) {
  while(1) {
    auto ca = aa == ab ? 0 : *aa;
    auto cb = *b;
    if (ca != cb || ca == 0) return ca - cb;
    aa++;
    b++;
  }
}

// This is a _prefix_ matcher, do not use if you want strcmp
inline const char* match_text(const char* text, const char* a, const char* b) {
  auto c = a;
  for (;c < b && (*c == *text) && *text; c++, text++);
  if (*text) return nullptr;
  return c;
}

// This is a _prefix_ matcher, do not use if you want strcmp
inline const char* match_text(const char** texts, int text_count, const char* a, const char* b) {
  for (auto i = 0; i < text_count; i++) {
    if (auto t = match_text(texts[i], a, b)) {
      return t;
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
// Matcheroni needs some way to compare different types of atoms - for
// convenience, comparators for "const char" are provided here.
// Comparators should return <0 for a<b, ==0 for a==b, and >0 for a>b.

template<typename atom1, typename atom2>
inline int atom_cmp(void* ctx, atom1* a, atom2 b) {
  return int(*a - b);
}

template<>
inline int atom_cmp(void* ctx, const char* a, char b) {
  return int(*a - b);
}

//------------------------------------------------------------------------------
// Matcheroni will call atom_rewind() to indicate to the host app that matching
// an optional branch has failed - this can be used to clean up any
// intermediate data structures that were created during the failed partial
// match.

// By default this does nothing, override in your application as needed.

template<typename atom>
inline void atom_rewind(void* ctx, atom* a, atom* b) {
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
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(ctx, a, C) == 0) {
      return a + 1;
    } else {
      return Atom<rest...>::match(ctx, a, b);
    }
  }
};

template <auto C>
struct Atom<C> {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(ctx, a, C) == 0) {
      return a + 1;
    } else {
      return nullptr;
    }
  }
};

struct AnyAtom {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
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
  static atom* match(void* ctx, atom* a, atom* b) {
    // Do NOT use this here, we MUST be able to match Any<> at EOF
    //if (!a || a == b) return nullptr;
    auto c = P::match(ctx, a, b);
    if (c) {
      auto d = Seq<rest...>::match(ctx, c, b);
      if (d) {
        return d;
      }
      else {
        return nullptr;
      }
    }
    else {
      return nullptr;
    }
  }
};

template<typename P>
struct Seq<P> {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    // Do NOT use this here, we MUST be able to match Any<> at EOF
    //if (!a || a == b) return nullptr;
    return P::match(ctx, a, b);
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
  static atom* match(void* ctx, atom* a, atom* b) {
    auto c = P::match(ctx, a, b);
    if (c) {
      return c;
    }
    else {
      /*+*/atom_rewind(ctx, a, b);
      return Oneof<rest...>::match(ctx, a, b);
    }
  }
};

template <typename P>
struct Oneof<P> {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    return P::match(ctx, a, b);
  }
};

//------------------------------------------------------------------------------
// Zero-or-one 'optional' patterns, equivalent to M? in regex.

// Opt<Atom<'a'>>::match("abcd") == "bcd"
// Opt<Atom<'a'>>::match("bcde") == "bcde"

template<typename... rest>
struct Opt {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (a == b) return a;
    auto c = Oneof<rest...>::match(ctx, a, b);
    if (c) {
      return c;
    }
    else {
      /*+*/atom_rewind(ctx, a, b);
      return a;
    }
  }
};

//------------------------------------------------------------------------------
// Zero-or-more patterns, roughly equivalent to M* in regex.
// HOWEVER - Seq<Any<Atom<'a'>>, Atom<'a'>> (unlike "a*a" in regex) will
// _never_ match anything, as the first Any<> is greedy and consumes all 'a's
// without doing any backtracking.

// Any<Atom<'a'>>::match("aaaab") == "b"
// Any<Atom<'a'>>::match("bbbbc") == "bbbbc"

template <typename... rest>
struct Any {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a) return nullptr;
    if (a == b) return a;
    while(1) {
      auto c = Oneof<rest...>::match(ctx, a, b);
      if (c) {
        a = c;
      }
      else {
        /*+*/atom_rewind(ctx, a, b);
        break;
      }
    }
    return a;
  }
};

struct Nothing {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    return a;
  }
};

//------------------------------------------------------------------------------
// One-or-more patterns, equivalent to M+ in regex.

// Some<Atom<'a'>>::match("aaaab") == "b"
// Some<Atom<'a'>>::match("bbbbc") == nullptr

template<typename...rest>
struct Some {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    auto c = Any<rest...>::match(ctx, a, b);
    if (c == a) {
      return nullptr;
    }
    else {
      return c;
    }
  }
};

//------------------------------------------------------------------------------
// A sequence of optional items that must match in order if present.
// SeqOpt<Atom<'a'>, Atom<'b'>, Atom<'c'>> will match "a", "ab", and "abc" but
// not "bc" or "c".

template<typename P, typename... rest>
struct SeqOpt {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a) return nullptr;
    if (a == b) return a;
    auto c = Opt<P>::match(ctx, a, b);
    if (c == a) {
      return a;
    }
    else {
      return SeqOpt<rest...>::match(ctx, c, b);
    }
  }
};

template<typename P>
struct SeqOpt<P> {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a) return nullptr;
    if (a == b) return a;
    return Opt<P>::match(ctx, a, b);
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
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a) return nullptr;
    auto c = P::match(ctx, a, b);
    /*+*/atom_rewind(ctx, a, b);
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
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a) return nullptr;
    auto c = P::match(ctx, a, b);
    /*+*/atom_rewind(ctx, a, b);
    return c ? nullptr : a;
  }
};

//------------------------------------------------------------------------------
// Turns empty sequence matches into non-matches. Useful if you have
// "a OR b OR ab" patterns, as you can turn them into NonEmpty<Opt<A>, Opt<B>>.

// NotEmpty<Opt<Atom<'c'>>, Opt<Atom<'d'>>>::match("cq") == "q"
// NotEmpty<Opt<Atom<'c'>>, Opt<Atom<'d'>>>::match("dq") == "q"
// NotEmpty<Opt<Atom<'c'>>, Opt<Atom<'d'>>>::match("zq") == nullptr

template<typename... rest>
struct NotEmpty {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a) return nullptr;
    auto end = Seq<rest...>::match(ctx, a, b);
    if (end == a) {
      return nullptr;
    }
    else {
      return end;
    }
  }
};

//------------------------------------------------------------------------------
// Repetition, equivalent to P{N} in regex.

template<int N, typename P>
struct Rep {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    atom* c = a;
    for(auto i = 0; i < N; i++) {
      c = P::match(ctx, a, b);
      if (!c) break;
    }
    if (c) {
      return c;
    }
    else {
      return nullptr;
    }
  }
};

//------------------------------------------------------------------------------
// Repetition, equivalent to P{M,N} in regex.

template<int M, int N, typename P>
struct RepRange {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    atom* c = a;
    for(auto i = 0; i < M; i++) {
      c = P::match(ctx, a, b);
      if (!c) {
        return nullptr;
      }
    }
    for(auto i = 0; i < (N-M); i++) {
      auto d = P::match(ctx, a, b);
      if (!d) {
        return c;
      }
    }
    return c;
  }
};

//------------------------------------------------------------------------------
// Atom-not-in-set matcher, which is a bit faster than using
// Seq<Not<Atom<...>>, AnyAtom>

template <auto C, auto... rest>
struct NotAtom {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(ctx, a, C) == 0) {
      return nullptr;
    }
    else {
      return NotAtom<rest...>::match(ctx, a, b);
    }
  }
};

template <auto C>
struct NotAtom<C> {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(ctx, a, C) == 0) {
      return nullptr;
    }
    else {
      return a + 1;
    }
  }
};

//------------------------------------------------------------------------------
// Ranges of atoms, inclusive.

template<auto RA, decltype(RA) RB>
struct Range {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(ctx, a, RA) < 0) {
      return nullptr;
    }
    if (atom_cmp(ctx, a, RB) > 0) {
      return nullptr;
    }
    return a + 1;
  }
};

template<auto RA, decltype(RA) RB>
struct NotRange {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(ctx, a, RA) < 0) return a + 1;
    if (atom_cmp(ctx, a, RB) > 0) return a + 1;
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// Advances the cursor until the pattern matches or we hit EOF. Does _not_
// consume the pattern. Equivalent to Any<Seq<Not<M>,AnyAtom>>

template<typename P>
struct Until {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    atom* c = a;
    while(c < b) {
      if (auto end = P::match(ctx, c, b)) {
        atom_rewind(ctx, c, b);
        return c;
      }
      c++;
    }
    return nullptr;
  }
};

#if 0
// These patterns need better names and explanations...

// Match some P until we see T. No T = no match.
template<typename P, typename T>
struct SomeUntil {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    auto c = AnyUntil<P,T>::match(ctx, a, b);
    return c == a ? nullptr : c;
  }
};

// Match any P until we see T. No T = no match.
template<typename P, typename T>
struct AnyUntil {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    while(a < b) {
      if (T::match(ctx, a, b)) return a;
      if (auto end = P::match(ctx, a, b)) {
        a = end;
      }
      else {
        return nullptr;
      }
    }
    return nullptr;
  }
};

// Match some P unless it would match T.
template<typename P, typename T>
struct SomeUnless {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    auto c = AnyUnless<P,T>::match(ctx, a, b);
    return c == a ? nullptr : c;
  }
};

// Match any P unless it would match T.
template<typename P, typename T>
struct AnyUnless {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    while(a < b) {
      if (T::match(ctx, a, b)) return a;
      if (auto end = P::match(ctx, a, b)) {
        a = end;
      }
      else {
        return a;
      }
    }
    return nullptr;
  }
};
#endif

//------------------------------------------------------------------------------
// Reference to a global matcher functions.

// const char* my_special_matcher(const char* a, const char* b);
// using pattern = Ref<my_special_matcher>;

template <auto F>
struct Ref;

template<typename atom, atom* (*F)(void* ctx, atom* a, atom* b)>
struct Ref<F> {
  static atom* match(void* ctx, atom* a, atom* b) {
    return F(ctx, a, b);
  }
};

//------------------------------------------------------------------------------
// Reference to a member matcher function.
// You _MUST_ pass a pointer to a Foo in the 'ctx' parameter when using these.

// struct Foo {
//   const char* match(const char* a, const char* b);
// };
//
// using pattern = Ref<&Foo::match>;
// Foo my_foo;
// auto end = pattern::match(&my_foo, a, b);

template <typename T, typename atom, atom* (T::*F)(atom* a, atom* b)>
struct Ref<F>
{
  static atom* match(void* ctx, atom* a, atom* b) {
    return (((T*)ctx)->*F)(a, b);
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
  static atom* match(void* ctx, atom* a, atom* b) {
    auto c = P::match(ctx, a, b);
    if (!c) return nullptr;
    start = a;
    size = int(c - a);
    return c;
  }
};

template<typename P>
struct MatchBackref {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    atom* start = (atom*)(StoreBackref<P>::start);
    int size = StoreBackref<P>::size;
    if (a + size > b) return nullptr;
    for (int i = 0; i < size; i++) {
      if(atom_cmp(ctx, a + i, start[i]) != 0) return nullptr;
    }
    return a + size;
  }
};

//------------------------------------------------------------------------------
// Equivalent to Seq<ldelim, Any<body>, rdelim>, but tries to match rdelim
// before body which can save a lot of useless matching time.

template<typename ldelim, typename body, typename rdelim>
struct DelimitedBlock {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    atom* c = a;
    c = ldelim::match(ctx, a, b);
    if (c == nullptr) {
      return nullptr;
    }

    while(1) {
      if (auto end = rdelim::match(ctx, c, b)) {
        return end;
      }
      else if (auto end = body::match(ctx, c, b)) {
        c = end;
      }
      else {
        return nullptr;
      }
    }
  }
};

//------------------------------------------------------------------------------
// Equivalent to
// Seq<
//   ldelim,
//   Opt<comma_separated<body>>,
//   rdelim
// >;

template<typename ldelim, typename item, typename separator, typename rdelim>
struct DelimitedList {

  // Might be faster to do this in terms of comma_separated, etc?

  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    atom* c = ldelim::match(ctx, a, b);
    if (c == nullptr) {
      return nullptr;
    }

    while(c) {
      if (auto end = rdelim::match(ctx, c, b)) return end;
      c = item::match(ctx, c, b);
      if (auto end = rdelim::match(ctx, c, b)) return end;
      c = separator::match(ctx, c, b);
    }
    return nullptr;
  }

};

//------------------------------------------------------------------------------
// Matches newline and EOF, but does not advance past it.

struct EOL {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a) return nullptr;
    if (a == b) return a;
    if (*a == atom('\n')) return a;
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// Not a matcher, but a template helper that allows us to use strings as
// template parameters. The parameter behaves as a fixed-length character array
// that does ___NOT___ match the trailing null.

template<int N>
struct StringParam {
  constexpr StringParam(const char (&str)[N]) {
    for (int i = 0; i < N; i++) str_val[i] = str[i];
  }
  constexpr static auto str_len = N-1;
  char str_val[str_len + 1];
};

//------------------------------------------------------------------------------
// Matches string literals. Does ___NOT___ match the trailing null.

// Lit<"foo">::match("foobar") == "bar"

template<StringParam lit>
struct Lit {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (a + lit.str_len > b) return nullptr;

    for (int i = 0; i < lit.str_len; i++) {
      if (atom_cmp(ctx, a + i, lit.str_val[i])) {
        return nullptr;
      }
    }
    return a + lit.str_len;
  }
};


//------------------------------------------------------------------------------
// Not a matcher, just a convenience helper - searches for a pattern anywhere
// in the input span and returns offset/length of the match if found.

struct SearchResult {
  operator bool() const { return length > 0; }
  int offset;
  int length;
};

template<typename P>
struct Search {
  template<typename atom>
  static SearchResult search(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return {0,0};
    auto cursor = a;
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

//------------------------------------------------------------------------------
// Matches larger sets of atoms packed into a string literal.

// Charset<"abcdef">::match("defg") == "efg"

template<StringParam chars>
struct Charset {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    for (auto i = 0; i < chars.str_len; i++) {
      if (*a == chars.str_val[i]) {
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
  static atom* match(void* ctx, atom* a, atom* b) {
    if (P::match_key(a, b)) {
      return P::match(ctx, a, b);
    }
    else {
      return Map<rest...>::match(ctx, a, b);
    }
  }
};

template <typename P>
struct Map<P> {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    return P::match(ctx, a, b);
  }
};

template <typename K, typename V>
struct KeyVal {
  template<typename atom>
  static atom* match_key(void* ctx, atom* a, atom* b) {
    return K::match(ctx, a, b);
  }

  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    return V::match(ctx, a, b);
  }
};

//------------------------------------------------------------------------------

template<typename NodeType>
struct PatternWrapper {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    return NodeType::pattern::match(ctx, a, b);
  }
};

//------------------------------------------------------------------------------

#ifdef MATCHERONI_USE_NAMESPACE
}; // namespace Matcheroni
#endif

#endif // #ifndef __MATCHERONI_H__
