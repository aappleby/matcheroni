#pragma once

#include "Matcheroni.h"

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
  template<typename atom>
  using matcher = matcheroni::matcher<atom>;

  const char*   name   = nullptr;
  matcher<char> match  = nullptr;
  unsigned int  color  = 0;
  const char*   span_a = nullptr;
  const char*   span_b = nullptr;
};

Token next_token(const char* cursor, const char* end);
void dump_lexer_stats();
