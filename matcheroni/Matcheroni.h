#ifndef __MATCHERONI_H__
#define __MATCHERONI_H__

#include <typeinfo>
#include <string.h>
#include <stdio.h>

#define MATCHERONI_ENABLE_TRACE

void print_escaped(const char* s, int len, unsigned int color);

//------------------------------------------------------------------------------
// Not a matcher, but a template helper that allows us to use strings as
// template parameters. The parameter behaves as a fixed-length character array
// that does ___NOT___ include the trailing null.

template<int N>
struct StringParam {
  constexpr StringParam(const char (&str)[N]) {
    for (int i = 0; i < N; i++) str_val[i] = str[i];
  }
  constexpr static auto str_len = N-1;
  char str_val[str_len + 1];
};

// print_typeid_name(typeid(T).name());

inline void print_typeid_name(const char* name) {
  int name_len = 0;
  if (sscanf(name, "%d", &name_len)) {
    while((*name >= '0') && (*name <= '9')) name++;
    for (int i = 0; i < name_len; i++) {
      putc(name[i], stdout);
    }
  }
  else {
    printf("%s", name);
  }
}

inline static int trace_enabled = 1;
inline static int indent_depth = 0;

#ifdef MATCHERONI_ENABLE_TRACE

template<typename NT>
inline void print_class_name() {
  const char* name = typeid(NT).name();
  int name_len = 0;
  if (sscanf(name, "%d", &name_len)) {
    while((*name >= '0') && (*name <= '9')) name++;
  }

  for (int i = 0; i < name_len; i++) {
    putc(name[i], stdout);
  }
}

template<typename NT>
struct Trace {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    static constexpr int context_len = 40;
    if (trace_enabled) {
      printf("[");
      print_escaped(a->lex->span_a, context_len, 0x404040);
      printf("] ");
      for (int i = 0; i < indent_depth; i++) printf("|   ");
      print_class_name<NT>();
      printf("?\n");
    }

    indent_depth += 1;
    auto end = NT::match(ctx, a, b);
    indent_depth -= 1;

    if (trace_enabled) {
      printf("[");
      if (end) {
        int match_len = end->lex->span_a - a->lex->span_a;
        if (match_len > context_len) match_len = context_len;
        int left_len = context_len - match_len;
        print_escaped(a->lex->span_a, match_len,  0x60C000);
        print_escaped(end->lex->span_a, left_len, 0x404040);
      }
      else {
        print_escaped(a->lex->span_a, context_len, 0x2020A0);
      }
      printf("] ");
      for (int i = 0; i < indent_depth; i++) printf("|   ");
      print_class_name<NT>();
      printf(end ? " OK\n" : " XXX\n");
    }

    return end;
  }
};

#else

template<StringParam doco, typename NT>
struct Trace {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    return NT::match(ctx, a, b);
  }
};

#endif

template<typename NT>
struct TraceEnable {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    trace_enabled++;
    auto end = NT::match(ctx, a, b);
    trace_enabled--;
    return end;
  }
};



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

inline int cmp_span_lit(const char* aa, const char* ab, const char* b) {
  while(1) {
    auto ca = aa == ab ? 0 : *aa;
    auto cb = *b;
    if (ca != cb || ca == 0) return ca - cb;
    aa++;
    b++;
  }
}

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
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(*a, C) == 0) {
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
    if (atom_cmp(*a, C) == 0) {
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
    auto c = P::match(ctx, a, b);
    return c ? Seq<rest...>::match(ctx, c, b) : nullptr;
  }
};

template<typename P>
struct Seq<P> {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
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
    return c ? c : Oneof<rest...>::match(ctx, a, b);
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
    auto c = Oneof<rest...>::match(ctx, a, b);
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

template <typename... rest>
struct Any {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    while(auto c = Oneof<rest...>::match(ctx, a, b)) {
      a = c;
    }
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
    auto c = Any<rest...>::match(ctx, a, b);
    return c == a ? nullptr : c;
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
    auto c = Opt<P>::match(ctx, a, b);
    return (c == a) ? a : SeqOpt<rest...>::match(ctx, c, b);
  }
};

template<typename P>
struct SeqOpt<P> {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    return Opt<P>::match(ctx, a, b);
  }
};

//------------------------------------------------------------------------------

template<typename P>
struct NonEmpty {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    auto c = P::match(ctx, a, b);
    return c == a ? nullptr : c;
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
    auto c = P::match(ctx, a, b);
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
    auto c = P::match(ctx, a, b);
    return c ? nullptr : a;
  }
};

//------------------------------------------------------------------------------
// Repetition, equivalent to M{N} in regex.

template<int N, typename P>
struct Rep {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    for(auto i = 0; i < N; i++) {
      auto c = P::match(ctx, a, b);
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
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(*a, C) == 0) return nullptr;
    return NotAtom<rest...>::match(ctx, a, b);
  }
};

template <auto C>
struct NotAtom<C> {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    return atom_cmp(*a, C) == 0 ? nullptr : a + 1;
  }
};

//------------------------------------------------------------------------------
// Ranges of atoms, inclusive.

template<auto RA, decltype(RA) RB>
struct Range {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
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
  static atom* match(void* ctx, atom* a, atom* b) {
    while(a < b) {
      if (P::match(ctx, a, b)) return a;
      a++;
    }
    return nullptr;
  }
};

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

// struct Foo {
//   const char* match(const char* a, const char* b);
// };
//
// using pattern = Ref<&Foo::match>;

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
      if(atom_cmp(a[i], start[i]) != 0) return nullptr;
    }
    return a + size;
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
// Matches string literals. Does ___NOT___ include the trailing null.

// Lit<"foo">::match("foobar") == "bar"

template<StringParam lit>
struct Lit {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (a + lit.str_len > b) return nullptr;

    for (auto i = 0; i < lit.str_len; i++) {
      if (a[i] != lit.str_val[i]) return nullptr;
    }
    return a + lit.str_len;
  }
};


//------------------------------------------------------------------------------
// Matches string literals as if they were atoms. Does ___NOT___ include the
// trailing null.
// You'll need to define atom_cmp(atom& a, StringParam<N>& b) to use this.

template<StringParam lit>
struct Keyword {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
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

template<typename NT>
struct PatternWrapper {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    return NT::pattern::match(ctx, a, b);
  }
};

//------------------------------------------------------------------------------

#ifdef MATCHERONI_USE_NAMESPACE
}; // namespace Matcheroni
#endif

#endif // #ifndef __MATCHERONI_H__
