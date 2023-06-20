#pragma once
#include <assert.h>
#include "Lexemes.h"
#include "Matcheroni.h"

//------------------------------------------------------------------------------

struct Token {
  Token(const Lexeme* lex) {
    assert((lex->type == LEX_INVALID) ^ (lex != nullptr));
    this->lex = lex;
  }

  static Token invalid() { return Token(nullptr); }

  //----------------------------------------

  bool is_valid() const {
    return lex && lex->type != LEX_INVALID;
  }

  bool is_punct() const {
    return lex && lex->is_punct();
  }

  bool is_punct(char p) const {
    //return lex && lex->is_punct(p);
    return lex && lex->span_a[0] == p;
  }

  bool is_lit(const char* lit) const {
    return lex && lex->is_lit(lit);
  }

  bool is_identifier(const char* lit = nullptr) const {
    return lex && lex->is_identifier(lit);
  }

  bool is_case_label() const {
    if (!lex) return false;
    if (lex->is_identifier("case")) return true;
    if (lex->is_identifier("default")) return true;
    return false;
  }

  bool is_constant() const {
    return (lex->type == LEX_FLOAT) || (lex->type == LEX_INT) || (lex->type == LEX_CHAR) || (lex->type == LEX_STRING);
  }

  //----------------------------------------

  const Lexeme* lex;
};

//------------------------------------------------------------------------------

template<StringParam lit>
struct Operator {

  static const Token* match(const Token* a, const Token* b) {
    if (!a || a == b) return nullptr;
    if (a + sizeof(lit.value) > b) return nullptr;

    for (auto i = 0; i < lit.len; i++) {
      if (!a[i].is_punct(lit.value[i])) return nullptr;
    }

    return a + sizeof(lit.value);
  }
};

//------------------------------------------------------------------------------

inline bool atom_eq(const Token& a, const LexemeType& b) {
  return a.lex->type == b;
}

inline bool atom_eq(const Token& a, const char& b) {
  return a.lex->len() == 1 && (*a.lex->span_a == b);
}

template<int N>
inline bool atom_eq(const Token& a, const StringParam<N>& b) {
  if (a.lex->len() != b.len) return false;
  for (auto i = 0; i < b.len; i++) {
    if (a.lex->span_a[i] != b.value[i]) return false;
  }

  return true;
}

//------------------------------------------------------------------------------

inline const Token* find_matching_delim(char ldelim, char rdelim, const Token* a, const Token* b) {
  if (*a->lex->span_a != ldelim) return nullptr;
  a++;

  while(a && a < b) {
    if (a->is_punct(rdelim)) return a;

    // Note that we _don't_ recurse through <> because they're not guaranteed
    // to be delimiters. Annoying aspect of C. :/

    if (a && a->is_punct('(')) a = find_matching_delim('(', ')', a, b);
    if (a && a->is_punct('{')) a = find_matching_delim('{', '}', a, b);
    if (a && a->is_punct('[')) a = find_matching_delim('[', ']', a, b);

    if (!a) return nullptr;
    a++;
  }

  return nullptr;
}

//------------------------------------------------------------------------------
// The Delimited<> modifier constrains a matcher to fit exactly between a pair
// of matching delimiters.
// For example, Delimited<'(', ')', NodeConstant> will match "(1)" but not
// "(1 + 2)".

template<char ldelim, char rdelim, typename P>
struct Delimited {
  static const Token* match(const Token* a, const Token* b) {
    if (!a || !a->is_punct(ldelim)) return nullptr;
    auto new_b = find_matching_delim(ldelim, rdelim, a, b);
    if (!new_b || !new_b->is_punct(rdelim)) return nullptr;

    if (!new_b) return nullptr;
    if (auto end = P::match(a + 1, new_b)) {
      if (end != new_b) return nullptr;
      return new_b + 1;
    }
    else {
      return nullptr;
    }
  }
};

//------------------------------------------------------------------------------

template<typename P>
using comma_separated = Seq<P, Any<Seq<Atom<','>, P>>, Opt<Atom<','>> >;

template<typename P>
using opt_comma_separated = Opt<comma_separated<P>>;

//------------------------------------------------------------------------------
