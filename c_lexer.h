#pragma once

#include "Matcheroni.h"

//------------------------------------------------------------------------------

enum LexemeType {
  LEX_INVALID = 0,
  LEX_SPACE,
  LEX_NEWLINE,
  LEX_STRING,
  LEX_RAW_STRING,
  LEX_ID,
  LEX_MLCOMMENT,
  LEX_SLCOMMENT,
  LEX_PREPROC,
  LEX_FLOAT,
  LEX_INT,
  LEX_PUNCT,
  LEX_CHAR,
  LEX_SPLICE,
  LEX_FORMFEED,
  LEX_ROOT,
};

inline const char* lex_to_str(LexemeType l) {
  switch(l) {
    case LEX_INVALID:    return "LEX_INVALID";
    case LEX_SPACE:      return "LEX_SPACE";
    case LEX_NEWLINE:    return "LEX_NEWLINE";
    case LEX_STRING:     return "LEX_STRING";
    case LEX_RAW_STRING: return "LEX_RAW_STRING";
    case LEX_ID:         return "LEX_ID";
    case LEX_MLCOMMENT:  return "LEX_MLCOMMENT";
    case LEX_SLCOMMENT:  return "LEX_SLCOMMENT";
    case LEX_PREPROC:    return "LEX_PREPROC";
    case LEX_FLOAT:      return "LEX_FLOAT";
    case LEX_INT:        return "LEX_INT";
    case LEX_PUNCT:      return "LEX_PUNCT";
    case LEX_CHAR:       return "LEX_CHAR";
    case LEX_SPLICE:     return "LEX_SPLICE";
    case LEX_FORMFEED:   return "LEX_FORMFEED";
    case LEX_ROOT:       return "LEX_ROOT";
  }
  return "<invalid>";
}

//------------------------------------------------------------------------------

const char* match_space             (const char* a, const char* b, void* ctx);
const char* match_newline           (const char* a, const char* b, void* ctx);
const char* match_header_name       (const char* a, const char* b, void* ctx);
const char* match_string_literal    (const char* a, const char* b, void* ctx);
const char* match_raw_string_literal(const char* a, const char* b, void* ctx);
const char* match_identifier        (const char* a, const char* b, void* ctx);
const char* match_multiline_comment (const char* a, const char* b, void* ctx);
const char* match_oneline_comment   (const char* a, const char* b, void* ctx);
const char* match_preproc           (const char* a, const char* b, void* ctx);
const char* match_float             (const char* a, const char* b, void* ctx);
const char* match_int               (const char* a, const char* b, void* ctx);
const char* match_punct             (const char* a, const char* b, void* ctx);
const char* match_character_constant(const char* a, const char* b, void* ctx);
const char* match_splice            (const char* a, const char* b, void* ctx);
const char* match_formfeed          (const char* a, const char* b, void* ctx);

struct Token {
  operator bool() const {
    return lex != LEX_INVALID;
  }

  template<typename atom>
  using matcher = matcheroni::matcher<atom>;
  LexemeType       lex    = LEX_INVALID;
  const char*   name   = nullptr;
  matcher<char> match  = nullptr;
  unsigned int  color  = 0;
  const char*   span_a = nullptr;
  const char*   span_b = nullptr;
};

Token next_token(const char* cursor, const char* end);
void dump_lexer_stats();
