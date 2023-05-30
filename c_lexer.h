#pragma once

#include "Parseroni.h"

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

Lexeme next_lexeme(const char* cursor, const char* end);
void dump_lexer_stats();

//------------------------------------------------------------------------------
