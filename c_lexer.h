#pragma once

const char* match_space             (const char* a, const char* b, void* ctx);
const char* match_newline           (const char* a, const char* b, void* ctx);
const char* match_character_constant(const char* a, const char* b, void* ctx);
const char* match_punct             (const char* a, const char* b, void* ctx);
const char* match_oneline_comment   (const char* a, const char* b, void* ctx);
const char* match_multiline_comment (const char* a, const char* b, void* ctx);
const char* match_nested_comment    (const char* a, const char* b, void* ctx);
const char* match_string_literal    (const char* a, const char* b, void* ctx);
const char* match_raw_string_literal(const char* a, const char* b, void* ctx);
const char* match_identifier        (const char* a, const char* b, void* ctx);
const char* match_preproc           (const char* a, const char* b, void* ctx);
const char* match_int               (const char* a, const char* b, void* ctx);
const char* match_float             (const char* a, const char* b, void* ctx);
const char* match_escape_sequence   (const char* a, const char* b, void* ctx);
const char* match_splice            (const char* a, const char* b, void* ctx);
const char* match_utf8              (const char* a, const char* b, void* ctx);
const char* match_utf8_bom          (const char* a, const char* b, void* ctx);
const char* match_header_name       (const char* a, const char* b, void* ctx);
