#pragma once

#include "Matcheroni.h"
#include <assert.h>

enum LexemeType {
  LEX_INVALID = 0,
  LEX_SPACE,
  LEX_NEWLINE,
  LEX_STRING,
  LEX_IDENTIFIER,
  LEX_COMMENT,
  LEX_PREPROC,
  LEX_FLOAT,
  LEX_INT,
  LEX_PUNCT,
  LEX_CHAR,
  LEX_SPLICE,
  LEX_FORMFEED,
  LEX_EOF,
};

//------------------------------------------------------------------------------

struct Lexeme {
  Lexeme(LexemeType lexeme, const char* span_a, const char* span_b) {
    assert((lexeme == LEX_INVALID) ^ (span_a != nullptr));
    assert((lexeme == LEX_INVALID) ^ (span_b != nullptr));
    this->lexeme = lexeme;
    this->span_a = span_a;
    this->span_b = span_b;
  }

  bool is_valid() const {
    return lexeme != LEX_INVALID;
  }

  bool is_eof() const {
    return lexeme == LEX_EOF;
  }

  bool is_gap() const {
    switch(lexeme) {
      case LEX_NEWLINE:
      case LEX_SPACE:
      case LEX_COMMENT:
      case LEX_SPLICE:
      case LEX_FORMFEED:
        return true;
    }
    return false;
  }

  const char* str() const {
    switch(lexeme) {
      case LEX_INVALID:    return "LEX_INVALID";
      case LEX_SPACE:      return "LEX_SPACE";
      case LEX_NEWLINE:    return "LEX_NEWLINE";
      case LEX_STRING:     return "LEX_STRING";
      case LEX_IDENTIFIER: return "LEX_IDENTIFIER";
      case LEX_COMMENT:    return "LEX_COMMENT";
      case LEX_PREPROC:    return "LEX_PREPROC";
      case LEX_FLOAT:      return "LEX_FLOAT";
      case LEX_INT:        return "LEX_INT";
      case LEX_PUNCT:      return "LEX_PUNCT";
      case LEX_CHAR:       return "LEX_CHAR";
      case LEX_SPLICE:     return "LEX_SPLICE";
      case LEX_FORMFEED:   return "LEX_FORMFEED";
      case LEX_EOF:        return "LEX_EOF";
    }
    return "<invalid>";
  }

  int len() const {
    return int(span_b - span_a);
  }

  //----------------------------------------

  bool is_punct() const {
    return (lexeme == LEX_PUNCT);
  }

  bool is_punct(char p) const {
    return (lexeme == LEX_PUNCT) && (*span_a == p);
  }

  bool is_lit(const char* lit) const {
    auto c = span_a;
    for (;c < span_b && (*c == *lit) && *lit; c++, lit++);
    return *lit ? false : true;
  }

  bool is_identifier(const char* lit = nullptr) const {
    return (lexeme == LEX_IDENTIFIER) && (lit == nullptr || is_lit(lit));
  }

  bool is_constant() const {
    return (lexeme == LEX_FLOAT) || (lexeme == LEX_INT) || (lexeme == LEX_CHAR) || (lexeme == LEX_STRING);
  }

  //----------------------------------------

  static Lexeme invalid() { return Lexeme(LEX_INVALID, nullptr, nullptr); }

  LexemeType    lexeme;
  const char*   span_a;
  const char*   span_b;
};

//------------------------------------------------------------------------------

const char* match_space      (const char* a, const char* b, void* ctx);
const char* match_newline    (const char* a, const char* b, void* ctx);
const char* match_string     (const char* a, const char* b, void* ctx);
const char* match_identifier (const char* a, const char* b, void* ctx);
const char* match_comment    (const char* a, const char* b, void* ctx);
const char* match_preproc    (const char* a, const char* b, void* ctx);
const char* match_float      (const char* a, const char* b, void* ctx);
const char* match_int        (const char* a, const char* b, void* ctx);
const char* match_punct      (const char* a, const char* b, void* ctx);
const char* match_char       (const char* a, const char* b, void* ctx);
const char* match_splice     (const char* a, const char* b, void* ctx);
const char* match_formfeed   (const char* a, const char* b, void* ctx);
const char* match_eof        (const char* a, const char* b, void* ctx);

const char* match_binary_op  (const char* a, const char* b, void* ctx);
const char* match_unary_op   (const char* a, const char* b, void* ctx);

Lexeme next_lexeme(const char* cursor, const char* end);
void dump_lexer_stats();

//------------------------------------------------------------------------------
