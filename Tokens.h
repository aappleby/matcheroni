#pragma once
#include <assert.h>
#include "Lexemes.h"

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

  bool is_eof() const {
    return type == TOK_EOF;
  }

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

  bool is_constant() const {
    return (type == TOK_FLOAT) || (type == TOK_INT) || (type == TOK_CHAR) || (type == TOK_STRING);
  }

  //----------------------------------------

  TokenType type;
  const Lexeme* lex;
};

//------------------------------------------------------------------------------
