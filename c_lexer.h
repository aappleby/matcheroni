#pragma once

const char* match_space             (const char* text, void* ctx);
const char* match_newline           (const char* text, void* ctx);
const char* match_character_constant(const char* text, void* ctx);
const char* match_punct             (const char* text, void* ctx);
const char* match_oneline_comment   (const char* text, void* ctx);
const char* match_multiline_comment (const char* text, void* ctx);
const char* match_nested_comment    (const char* text, void* ctx);
const char* match_string_literal    (const char* text, void* ctx);
const char* match_raw_string_literal(const char* text, void* ctx);
const char* match_identifier        (const char* text, void* ctx);
const char* match_preproc           (const char* text, void* ctx);
const char* match_int               (const char* text, void* ctx);
const char* match_float             (const char* text, void* ctx);
const char* match_escape_sequence   (const char* text, void* ctx);
const char* match_splice            (const char* text, void* ctx);
const char* match_utf8              (const char* text, void* ctx);
const char* match_utf8_bom          (const char* text, void* ctx);

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
