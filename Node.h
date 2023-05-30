#pragma once
#include <assert.h>
#include "c_lexer.h"

struct Node;

void dump_tree(Node* head, int max_depth = 1000, int indentation = 0);

//------------------------------------------------------------------------------

enum NodeType {
  NODE_INVALID = 0,
  NODE_TRANSLATION_UNIT,

  NODE_DECLARATION_SPECIFIER,
  NODE_DECLARATION_SPECIFIERS,

  NODE_PREPROC,
  NODE_IDENTIFIER,
  NODE_CONSTANT,

  NODE_TYPEDEF_NAME,
  NODE_TYPEOF_SPECIFIER,
  NODE_DECLTYPE,
  NODE_DECLARATION,

  NODE_CLASS_SPECIFIER,
  NODE_FIELD_DECLARATION_LIST,

  NODE_ACCESS_SPECIFIER,
  NODE_COMPOUND_STATEMENT,
  NODE_PARAMETER_LIST,
  NODE_CONSTRUCTOR,

  NODE_TEMPLATE_DECLARATION,
  NODE_TEMPLATE_PARAMETER_LIST,

  NODE_PUNCT,

  /*
  BLOCK_PAREN,
  BLOCK_BRACK,
  BLOCK_BRACE,

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

inline const char* tok_to_str(NodeType t) {
  switch(t) {
    case NODE_INVALID:          return "NODE_INVALID";
    case NODE_TRANSLATION_UNIT: return "NODE_TRANSLATION_UNIT";
    case NODE_PREPROC:          return "NODE_PREPROC";
    case NODE_IDENTIFIER:       return "NODE_IDENTIFIER";
    case NODE_CONSTANT:         return "NODE_CONSTANT";

    //case BLOCK_PAREN: return "BLOCK_PAREN";
    //case BLOCK_BRACK: return "BLOCK_BRACK";
    //case BLOCK_BRACE: return "BLOCK_BRACE";
  }
  return "<token>";
}

//------------------------------------------------------------------------------

struct Node {

  Node(NodeType node_type, const Lexeme* lex_a, const Lexeme* lex_b) {
    assert(node_type != NODE_INVALID);
    if (lex_a == nullptr && lex_b == nullptr) {
    }
    else {
      assert(lex_b > lex_a);
    }

    this->node_type = node_type;
    this->lex_a = lex_a;
    this->lex_b = lex_b;
  };

  ~Node() {
    auto cursor = head;
    while(cursor) {
      auto next = cursor->next;
      delete cursor;
      cursor = next;
    }
  }

  NodeType node_type;
  const Lexeme* lex_a;
  const Lexeme* lex_b;

  Node* parent = nullptr;
  Node* prev   = nullptr;
  Node* next   = nullptr;
  Node* head   = nullptr;
  Node* tail   = nullptr;

  //----------------------------------------

  Node* child(int index) {
    auto cursor = head;
    for (auto i = 0; i < index; i++) {
      if (cursor) cursor = cursor->next;
    }
    return cursor;
  }

  //----------------------------------------

  void append(Node* tok) {
    tok->parent = this;

    if (tail) {
      tail->next = tok;
      tok->prev = tail;
      tail = tok;
    }
    else {
      head = tok;
      tail = tok;
    }
  }

  //----------------------------------------

  void sanity() {
    // All our children should be sane.
    for (auto cursor = head; cursor; cursor = cursor->next) {
      cursor->sanity();
    }

    // Our prev/next pointers should be hooked up correctly
    assert(!next || next->prev == this);
    assert(!prev || prev->next == this);

    //assert(!lex_prev || lex_prev->lex_next == this);
    ///assert(!lex_next || lex_next->lex_prev == this);


    // Our span should exactly enclose our children.
    //assert(!lex_prev || lex_prev->span_b == span_a);
    //assert(!lex_next || lex_next->span_a == span_b);
    //assert(!tok_head || tok_head->span_a == span_a);
    //assert(!tok_tail || tok_tail->span_b == span_b);

    Node* cursor = nullptr;

    // Check token chain
    for (cursor = head; cursor && cursor != tail; cursor = cursor->next);
    assert(cursor == tail);

    for (cursor = tail; cursor && cursor != head; cursor = cursor->prev);
    assert(cursor == head);
  }

  //----------------------------------------

  void dump_tree() {
  }

};

//------------------------------------------------------------------------------
