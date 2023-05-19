#pragma once

const char* match_space(const char* text, void* ctx);
const char* match_newline(const char* text, void* ctx);
const char* match_indentation(const char* text, void* ctx);
const char* match_char_literal(const char* text, void* ctx);
const char* match_punct(const char* text, void* ctx);
const char* match_oneline_comment(const char* text, void* ctx);
const char* match_multiline_comment(const char* text, void* ctx);
const char* match_nested_comment(const char* text, void* ctx);
const char* match_ws(const char* text, void* ctx);
const char* match_string(const char* text, void* ctx);
const char* match_raw_string(const char* text, void* ctx);
const char* match_identifier(const char* text, void* ctx);
const char* match_angle_path(const char* text, void* ctx);
const char* match_preproc(const char* text, void* ctx);
const char* match_int(const char* text, void* ctx);
const char* match_float(const char* text, void* ctx);
const char* match_escape(const char* text, void* ctx);
const char* match_splice(const char* text, void* ctx);
const char* match_utf8(const char* text, void* ctx);
const char* match_bom(const char* text, void* ctx);
