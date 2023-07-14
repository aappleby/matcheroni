// To build a parse tree, we wrap the patterns we want to create nodes for
// in a Capture<> matcher that will invoke our node factory. We can also wrap
// them in a Trace<> matcher if we want to debug our patterns.

#if 0
# Comparisons of Matcheroni's JSON parser against...

Lexy
 - https://github.com/foonathan/lexy/blob/main/examples/json.cpp
 - Lexy seems much more verbose.

PEGTL
 - https://github.com/taocpp/PEGTL/blob/main/include/tao/pegtl/contrib/json.hpp
 - PEGTL is very similar to Matcheroni.

Boost.Spirit
 - https://gist.github.com/legnaleurc/3031094
 - json example using other biggish chunks of spirit parser
 - harder to see how it fits together
 - rules are member variables of a struct?
#endif


// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

//------------------------------------------------------------------------------
// Does this actually work?

/*
struct AngleSpan;
struct BraceSpan;
struct BrackSpan;
struct ParenSpan;
struct DQuoteSpan;
struct SQuoteSpan;

using AnySpan = Oneof<AngleSpan, BraceSpan, BrackSpan, ParenSpan, DQuoteSpan, SQuoteSpan>;

template<auto ldelim, auto rdelim>
using DelimitedSpan =
Seq<
  Atom<ldelim>,
  Any<
    AnySpan,
    NotAtom<rdelim>
  >,
  Atom<rdelim>
>;

struct AngleSpan : public PatternWrapper<AngleSpan> {
  using pattern = DelimitedSpan<'<', '>'>;
};

struct BraceSpan : public PatternWrapper<BraceSpan> {
  using pattern = DelimitedSpan<'{', '}'>;
};

struct BrackSpan : public PatternWrapper<BrackSpan> {
  using pattern = DelimitedSpan<'[', ']'>;
};

struct ParenSpan : public PatternWrapper<ParenSpan> {
  using pattern = DelimitedSpan<'(', ')'>;
};

struct DQuoteSpan : public PatternWrapper<DQuoteSpan> {
  using pattern = DelimitedSpan<'"', '"'>;
};

struct SQuoteSpan : public PatternWrapper<SQuoteSpan> {
  using pattern = DelimitedSpan<'\'', '\''>;
};

inline const char* find_matching_delim(void* ctx, const char* a, const char* b) {
  return AnySpan::match(ctx, a, b);
}
*/

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

/*
inline Token* find_matching_delim(void* ctx, char ldelim, char rdelim, Token* a, Token* b) {
  if (a->as_str()[0] != ldelim) return nullptr;
  a++;

  while(a && a < b) {
    if (a->is_punct(rdelim)) return a;

    // Note that we _don't_ recurse through <> because they're not guaranteed
    // to be delimiters. Annoying aspect of C. :/

    if (a && a->is_punct('(')) a = find_matching_delim(ctx, '(', ')', a, b);
    if (a && a->is_punct('{')) a = find_matching_delim(ctx, '{', '}', a, b);
    if (a && a->is_punct('[')) a = find_matching_delim(ctx, '[', ']', a, b);

    if (!a) return nullptr;
    a++;
  }

  return nullptr;
}
*/

//------------------------------------------------------------------------------
// The Delimited<> modifier constrains a matcher to fit exactly between a pair
// of matching delimiters.
// For example, Delimited<'(', ')', NodeConstant> will match "(1)" but not
// "(1 + 2)".

/*
template<char ldelim, char rdelim, typename P>
struct Delimited {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || !a->is_punct(ldelim)) return nullptr;
    auto new_b = find_matching_delim(ctx, ldelim, rdelim, a, b);
    if (!new_b || !new_b->is_punct(rdelim)) return nullptr;

    if (!new_b) return nullptr;
    if (auto end = P::match(ctx, a + 1, new_b)) {
      if (end != new_b) return nullptr;
      return new_b + 1;
    }
    else {
      return nullptr;
    }
  }
};
*/

//------------------------------------------------------------------------------
/*
struct NodeDispenser {

  NodeDispenser(ParseNode** children, size_t child_count) {
    this->children = children;
    this->child_count = child_count;
  }

  template<typename P>
  operator P*() {
    if (child_count == 0) return nullptr;
    if (children[0] == nullptr) return nullptr;
    if (children[0]->is_a<P>()) {
      P* result = children[0]->as_a<P>();
      children++;
      child_count--;
      return result;
    }
    else {
      return nullptr;
    }
  }

  ParseNode** children;
  size_t child_count;
};
*/
