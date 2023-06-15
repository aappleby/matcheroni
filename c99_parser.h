#pragma once

#include "Tokens.h"
#include "Node.h"

struct NodeBase;

const Token* parse_declaration(const Token* a, const Token* b);
const Token* parse_external_declaration(const Token* a, const Token* b);
const Token* parse_expression (const Token* a, const Token* b);
const Token* parse_statement  (const Token* a, const Token* b);
const Token* parse_statement_compound  (const Token* a, const Token* b);

//------------------------------------------------------------------------------

template<typename P>
struct ProgressBar {
  static const Token* match(const Token* a, const Token* b) {
    printf("%.40s\n", a->lex->span_a);
    return P::match(a, b);
  }
};

struct NodeTranslationUnit : public NodeMaker<NodeTranslationUnit> {
  using pattern = Any<
    ProgressBar<
      Oneof<
        NodePreproc,
        Ref<parse_external_declaration>
      >
    >
  >;
};

//------------------------------------------------------------------------------
