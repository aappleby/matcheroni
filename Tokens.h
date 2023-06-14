#pragma once
#include <assert.h>
#include "Lexemes.h"
#include "Matcheroni.h"

//------------------------------------------------------------------------------

enum TokenType {
  TOK_INVALID = 0,
  TOK_STRING,
  TOK_IDENTIFIER,
  TOK_PREPROC,
  TOK_FLOAT,
  TOK_INT,
  TOK_PUNCT,
  TOK_CHAR,
  TOK_EOF,
};

//------------------------------------------------------------------------------

inline TokenType lex_to_tok(LexemeType lex) {
  switch(lex) {
    case LEX_STRING:     return TOK_STRING;
    case LEX_IDENTIFIER: return TOK_IDENTIFIER;
    case LEX_PREPROC:    return TOK_PREPROC;
    case LEX_FLOAT:      return TOK_FLOAT;
    case LEX_INT:        return TOK_INT;
    case LEX_PUNCT:      return TOK_PUNCT;
    case LEX_CHAR:       return TOK_CHAR;
    case LEX_EOF:        return TOK_EOF;
  }
  return TOK_INVALID;
}

//------------------------------------------------------------------------------

struct Token {
  Token(TokenType type, const Lexeme* lex) {
    assert((type == TOK_INVALID) ^ (lex != nullptr));
    this->type = type;
    this->lex = lex;
  }

  static Token invalid() { return Token(TOK_INVALID, nullptr); }

  const char* str() const {
    switch(type) {
      case TOK_INVALID:    return "TOK_INVALID";
      case TOK_STRING:     return "TOK_STRING";
      case TOK_IDENTIFIER: return "TOK_IDENTIFIER";
      case TOK_PREPROC:    return "TOK_PREPROC";
      case TOK_FLOAT:      return "TOK_FLOAT";
      case TOK_INT:        return "TOK_INT";
      case TOK_PUNCT:      return "TOK_PUNCT";
      case TOK_CHAR:       return "TOK_CHAR";
      case TOK_EOF:        return "TOK_EOF";
    }
    return "<invalid>";
  }

  //----------------------------------------

  bool is_valid() const {
    return type != TOK_INVALID;
  }

  bool is_punct() const {
    return lex->is_punct();
  }

  bool is_punct(char p) const {
    return lex && lex->is_punct(p);
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
    return (type == TOK_FLOAT) || (type == TOK_INT) || (type == TOK_CHAR) || (type == TOK_STRING);
  }

  //----------------------------------------

  TokenType type;
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

inline bool atom_eq(const Token& a, const TokenType& b) {
  return a.type == b;
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
using comma_separated = Seq<P,Any<Seq<Atom<','>, P>>>;

template<typename P>
using opt_comma_separated = Opt<comma_separated<P>>;

//------------------------------------------------------------------------------
