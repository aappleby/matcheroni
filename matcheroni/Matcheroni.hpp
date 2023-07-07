//------------------------------------------------------------------------------
// Matcheroni is a single-header, zero-dependency toolkit that makes building
// custom pattern matchers, lexers, and parsers easier. Matcheroni is based on
// "Parsing Expression Grammars" (PEGs), which are similar in concept to
// regular expressions but behave slightly differently.

// See https://en.wikipedia.org/wiki/Parsing_expression_grammar for details.

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#ifndef MATCHERONI_H_
#define MATCHERONI_H_

namespace matcheroni {

//------------------------------------------------------------------------------
// Matcheroni is based on building trees of "matcher" functions. A matcher
// function takes a range of "atoms" (could be characters, could be some
// application-specific type) as input and returns either the endpoint of the
// match if found, or nullptr if a match was not found.

// Matcher functions accept an opaque context pointer 'ctx', which can be used
// to pass in a pointer to application-specific state.

// Matcher functions must always handle null pointers and empty ranges.

template<typename atom>
using matcher_function = atom* (*) (void* ctx, atom* a, atom* b);

//------------------------------------------------------------------------------
// Matchers will often need to compare ranges of atoms against null-delimited
// strings ala strcmp(), so we provide this function for convenience.

inline int strcmp_span(const char* a, const char* b, const char* lit) {
  while(1) {
    auto ca = a == b ? 0 : *a;
    auto cb = *lit;
    if (ca != cb || ca == 0) return ca - cb;
    a++;
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

template<typename atom1, typename atom2>
inline int atom_cmp(void* ctx, atom1* a, atom2 b) {
  return int(*a - b);
}

//------------------------------------------------------------------------------
// Matcheroni also needs a way to tell the host application to "rewind" its
// state when an intermediate match fails - this can be used to clean up any
// intermediate data structures that were created during the failed partial
// match.

// By default this does nothing, but if you specialize this for your atom type
// Matcheroni will call that instead.

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

//------------------------------------------------------------------------------
// AnyAtom is equivalent to '.' in regex.

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
// 'Opt' matches 'optional' patterns, equivalent to '?' in regex.

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
// 'Any' matches zero or more copies of a pattern, equivalent to '*' in regex.

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
// 'Some' matches one or more copies of a pattern, equivalent to '+' in regex.

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
// The 'And' predicate matches a pattern but does _not_ advance the cursor.
// Used for lookahead.

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
// The 'Not' predicate is the logical negation of the 'And' predicate.

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
// 'SeqOpt' matches a sequence of items that are individually optional, but
// that must match in order if present.

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
// 'NotEmpty' turns empty sequence matches into non-matches. Useful if you have
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
// 'Rep' is equivalent to '{N}' in regex.

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
// 'RepRange' is equivalent '{M,N}' in regex.

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
// 'NotAtom' matches any atom that is _not_ in its argument list, which is a
// bit faster than using Seq<Not<Atom<...>>, AnyAtom>

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
// 'Range' matches ranges of atoms, equivalent to '[a-b]' in regex.

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

//------------------------------------------------------------------------------
// 'NotRange' matches ranges of atoms not in the given range, equivalent to
// '[^a-b]' in regex.

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
// 'Until' matches anything until we see the given pattern or we hit EOF.
// The pattern is _not_ consumed.

// Equivalent to Any<Seq<Not<M>,AnyAtom>>

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
// 'Ref' is used to call a user-defined matcher function from a Matcheroni
// pattern.

// 'Ref' can also be used to call member functions. You _MUST_ pass a pointer
// to an object via the 'ctx' parameter when using these.

// const char* my_special_matcher(const char* a, const char* b);
// using pattern = Ref<my_special_matcher>;

// struct Foo {
//   const char* match(const char* a, const char* b);
// };
//
// using pattern = Ref<&Foo::match>;
// Foo my_foo;
// auto end = pattern::match(&my_foo, a, b);

template <auto F>
struct Ref;

template<typename atom, atom* (*F)(void* ctx, atom* a, atom* b)>
struct Ref<F> {
  static atom* match(void* ctx, atom* a, atom* b) {
    return F(ctx, a, b);
  }
};

template <typename T, typename atom, atom* (T::*F)(atom* a, atom* b)>
struct Ref<F>
{
  static atom* match(void* ctx, atom* a, atom* b) {
    return (((T*)ctx)->*F)(a, b);
  }
};

//------------------------------------------------------------------------------
// 'StoreBackref/MatchBackref' stores and matches backreferences.
// These are currently used for raw string delimiters in the C lexer.

// Note that the backreference is stored as a static pointer in the
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
// 'DelimitedBlock' is equivalent to Seq<ldelim, Any<body>, rdelim>, but it
// tries to match rdelim before body which can save matching time.

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
// 'DelimitedList' is the same as 'DelimitedBlock' except that it adds a
// separator pattern between items.

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
// 'EOL' matches newline and EOF, but does not advance past it.

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
// 'Lit' matches string literals. Does ___NOT___ match the trailing null.

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
// 'Search' is not a matcher, just a convenience helper - searches for a
// pattern anywhere in the input span and returns offset/length of the match if
// found.

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
// 'Charset' matches larger sets of atoms packed into a string literal, which
// is more concise than Atom<'a','b','c'...> for large sets of atoms.

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

template<typename P, typename... rest>
struct Map {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (P::match_key(a, b)) {
      atom_rewind(ctx, a, b);
      return P::match(ctx, a, b);
    }
    else {
      atom_rewind(ctx, a, b);
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
// 'PatternWrapper' is just a convenience class that lets you do this:

// struct MyPattern : public PatternWrapper<MyPattern> {
//   using pattern = Atom<'a', 'b', 'c'>;
// };
//
// auto end = Some<MyPattern>::match(ctx, a, b);

template<typename T>
struct PatternWrapper {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    return T::pattern::match(ctx, a, b);
  }
};

//------------------------------------------------------------------------------

}; // namespace Matcheroni

#endif // #ifndef MATCHERONI_H_
