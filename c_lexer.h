#pragma once

const char* match_space(const char* text, void* ctx);
const char* match_newline(const char* text, void* ctx);
const char* match_indentation(const char* text, void* ctx);
const char* match_character_constant(const char* text, void* ctx);
const char* match_punct(const char* text, void* ctx);
const char* match_oneline_comment(const char* text, void* ctx);
const char* match_multiline_comment(const char* text, void* ctx);
const char* match_nested_comment(const char* text, void* ctx);
const char* match_string_literal(const char* text, void* ctx);
const char* match_raw_string_literal(const char* text, void* ctx);
const char* match_identifier(const char* text, void* ctx);
const char* match_preproc(const char* text, void* ctx);
const char* match_int(const char* text, void* ctx);
const char* match_float(const char* text, void* ctx);
const char* match_escape_sequence(const char* text, void* ctx);
const char* match_splice(const char* text, void* ctx);
const char* match_utf8(const char* text, void* ctx);
const char* match_utf8_bom(const char* text, void* ctx);
