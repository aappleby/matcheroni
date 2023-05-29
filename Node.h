#pragma once
#include <assert.h>
#include "c_lexer.h"

struct Node;

void dump_tree(Node* head, int max_depth = 1000, int indentation = 0);

//------------------------------------------------------------------------------

enum TokenType {
  TOK_INVALID = 0,
  TOK_ROOT,

  // Gap tokens
  TOK_SPACE,
  TOK_NEWLINE,
  TOK_SLCOMMENT,
  TOK_MLCOMMENT,
  TOK_SPLICE,

  TOK_ID,

  BLOCK_PAREN,
  BLOCK_BRACK,
  BLOCK_BRACE,

  /*
  TOK_NONE,
  TOK_HEADER_NAME,
  TOK_PREPROC,
  TOK_CONST, TOK_STRING,
  TOK_LPAREN, TOK_RPAREN,
  TOK_LBRACE, TOK_RBRACE,
  TOK_LBRACK, TOK_RBRACK,
  TOK_DOT, TOK_COMMA, TOK_ARROW, TOK_PLUSPLUS, TOK_MINUSMINUS,
  TOK_SIZEOF,
  TOK_PLUS, TOK_MINUS, TOK_AMPERSAND, TOK_STAR, TOK_TILDE, TOK_BANG,
  TOK_FSLASH, TOK_BSLASH, TOK_PERCENT,
  TOK_EQ, TOK_LT, TOK_GT, TOK_LTLT, TOK_GTGT, TOK_LTEQ, TOK_GTEQ, TOK_EQEQ, TOK_BANGEQ,
  TOK_STAREQ, TOK_FSLASHEQ, TOK_PERCENTEQ, TOK_PLUSEQ, TOK_MINUSEQ, TOK_LTLTEQ, TOK_GTGTEQ, TOK_AMPEQ, TOK_CARETEQ, TOK_PIPEEQ,
  TOK_CARET, TOK_PIPE, TOK_AMPAMP, TOK_PIPEPIPE, TOK_QUEST, TOK_COLON,
  */
};

inline const char* tok_to_str(TokenType t) {
  switch(t) {
    case TOK_INVALID: return "TOK_INVALID";
    case TOK_ROOT: return "TOK_ROOT";
    case TOK_SPACE: return "TOK_SPACE";
    case TOK_NEWLINE: return "TOK_NEWLINE";
    case TOK_SLCOMMENT: return "TOK_SLCOMMENT";
    case TOK_MLCOMMENT: return "TOK_MLCOMMENT";
    case TOK_SPLICE: return "TOK_SPLICE";

    case TOK_ID: return "TOK_ID";

    case BLOCK_PAREN: return "BLOCK_PAREN";
    case BLOCK_BRACK: return "BLOCK_BRACK";
    case BLOCK_BRACE: return "BLOCK_BRACE";
  }
  return "<token>";
}

//------------------------------------------------------------------------------

struct Lexeme {
  LexemeType  lexeme;
  const char* span_a; // First char
  const char* span_b; // Last char + 1
};

//------------------------------------------------------------------------------

struct Node {

  Node(LexemeType lexeme, TokenType token, const char* span_a, const char* span_b) {
    this->lexeme = lexeme;
    this->token  = token;
    this->span_a = span_a;
    this->span_b = span_b;
  };

  LexemeType     lexeme;
  TokenType   token;
  const char* span_a; // First char
  const char* span_b; // Last char + 1

  Node* parent   = nullptr; // Parent node

  // Lexeme tree
  Node* lex_prev = nullptr; // Prev lexeme
  Node* lex_next = nullptr; // Next lexeme
  Node* lex_head = nullptr; // First sub-lexeme
  Node* lex_tail = nullptr; // Last sub-lexeme

  // Token tree
  Node* tok_prev = nullptr; // Prev token
  Node* tok_next = nullptr; // Next token
  Node* tok_head = nullptr; // First sub-token
  Node* tok_tail = nullptr; // Last sub-token

  //----------------------------------------

  bool is_gap() {
    switch(lexeme) {
      case LEX_NEWLINE:
      case LEX_SPACE:
      case LEX_SLCOMMENT:
      case LEX_MLCOMMENT:
      case LEX_SPLICE:
      case LEX_FORMFEED:
        return true;
    }
    return false;
  }

  //----------------------------------------

  void append_lex(Node* lex) {
    lex->parent = this;

    if (lex_tail) {
      lex_tail->lex_next = lex;
      lex->lex_prev = lex_tail;
      lex_tail = lex;
    }
    else {
      lex_head = lex;
      lex_tail = lex;
    }
  }

  void append_tok(Node* tok) {
    tok->parent = this;

    if (tok_tail) {
      tok_tail->tok_next = tok;
      tok->tok_prev = tok_tail;
      tok_tail = tok;
    }
    else {
      tok_head = tok;
      tok_tail = tok;
    }
  }

  //----------------------------------------

  void sanity() {
    // All our children should be sane.
    for (auto cursor = lex_head; cursor; cursor = cursor->lex_next) {
      cursor->sanity();
    }

    // Our prev/next pointers should be hooked up correctly
    assert(!tok_next || tok_next->tok_prev == this);
    assert(!tok_prev || tok_prev->tok_next == this);

    assert(!lex_prev || lex_prev->lex_next == this);
    assert(!lex_next || lex_next->lex_prev == this);


    // Our span should exactly enclose our children.
    assert(!lex_prev || lex_prev->span_b == span_a);
    assert(!lex_next || lex_next->span_a == span_b);
    assert(!tok_head || tok_head->span_a == span_a);
    assert(!tok_tail || tok_tail->span_b == span_b);

    Node* cursor = nullptr;

    // Check token chain
    for (cursor = tok_head; cursor && cursor != tok_tail; cursor = cursor->tok_next);
    assert(cursor == tok_tail);

    for (cursor = tok_tail; cursor && cursor != tok_head; cursor = cursor->tok_prev);
    assert(cursor == tok_head);

    // Check lexeme chain
    for (cursor = lex_head; cursor && cursor != lex_tail; cursor = cursor->lex_next);
    assert(cursor == lex_tail);

    for (cursor = lex_tail; cursor && cursor != lex_head; cursor = cursor->lex_prev);
    assert(cursor == lex_head);
  }
};

//------------------------------------------------------------------------------

#if 0
struct NodeList {
  Node* head_tok = nullptr;
  Node* tail_tok = nullptr;

  Node* head_sib = nullptr;
  Node* tail_sib = nullptr;

  void sanity(const char* span_a, const char* span_b) {
    if (!head_tok) return;
    Node* cursor = nullptr;

    assert(head_tok->span_a == span_a);
    assert(tail_tok->span_b == span_b);

    for (cursor = head_tok; cursor && cursor != tail_tok; cursor = cursor->next_lex) {
      assert(cursor->span_b == cursor->next_lex->span_a);
    }
    assert(cursor == tail_tok);

    for (cursor = tail_tok; cursor && cursor != head_tok; cursor = cursor->prev_lex) {
      assert(cursor->span_a == cursor->prev_lex->span_b);
    }
    assert(cursor == head_tok);
  }
};
#endif

//------------------------------------------------------------------------------
