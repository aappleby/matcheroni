#pragma once

const char* rmatch_space             (const char* a, const char* b, void* ctx);
const char* rmatch_newline           (const char* a, const char* b, void* ctx);
const char* rmatch_character_constant(const char* a, const char* b, void* ctx);
const char* rmatch_punct             (const char* a, const char* b, void* ctx);
const char* rmatch_oneline_comment   (const char* a, const char* b, void* ctx);
const char* rmatch_multiline_comment (const char* a, const char* b, void* ctx);
const char* rmatch_nested_comment    (const char* a, const char* b, void* ctx);
const char* rmatch_string_literal    (const char* a, const char* b, void* ctx);
const char* rmatch_raw_string_literal(const char* a, const char* b, void* ctx);
const char* rmatch_identifier        (const char* a, const char* b, void* ctx);
const char* rmatch_preproc           (const char* a, const char* b, void* ctx);
const char* rmatch_int               (const char* a, const char* b, void* ctx);
const char* rmatch_float             (const char* a, const char* b, void* ctx);
const char* rmatch_escape_sequence   (const char* a, const char* b, void* ctx);
const char* rmatch_splice            (const char* a, const char* b, void* ctx);
const char* rmatch_utf8              (const char* a, const char* b, void* ctx);
const char* rmatch_utf8_bom          (const char* a, const char* b, void* ctx);
