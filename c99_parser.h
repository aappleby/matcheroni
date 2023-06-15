#pragma once

struct NodeBase;
struct Token;

const Token* parse_declaration(const Token* a, const Token* b);
const Token* parse_definition (const Token* a, const Token* b);
const Token* parse_expression (const Token* a, const Token* b);
const Token* parse_preproc    (const Token* a, const Token* b);
const Token* parse_statement  (const Token* a, const Token* b);
const Token* parse_statement_compound  (const Token* a, const Token* b);
