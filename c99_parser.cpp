#include "Matcheroni.h"
#include <assert.h>
#include <stdio.h>
#include <memory.h>
#include "c_lexer.h"
#include "Node.h"

#include <stack>

using namespace matcheroni;

//------------------------------------------------------------------------------

const char* text = R"(
#include <stdio.h>

// This is a comment.

int main(int argc, char** argv) {
  // Another comment
  printf("Hello () World %d %p!\n", 12345, argv);
  return 0;
}

)";

//------------------------------------------------------------------------------

#if 0
//----------------------------------------
LEX_ROOT       TOK_ROOT       \n#include <stdio.
 |- LEX_PREPROC    TOK_INVALID    #include <stdio.h>
 |- LEX_ID         TOK_INVALID    int
 |- LEX_ID         TOK_INVALID    main
 |- LEX_PUNCT      TOK_INVALID    (
 |- LEX_ID         TOK_INVALID    int
 |- LEX_ID         TOK_INVALID    argc
 |- LEX_PUNCT      TOK_INVALID    ,
 |- LEX_ID         TOK_INVALID    char
 |- LEX_PUNCT      TOK_INVALID    *
 |- LEX_PUNCT      TOK_INVALID    *
 |- LEX_ID         TOK_INVALID    argv
 |- LEX_PUNCT      TOK_INVALID    )
 |- LEX_PUNCT      TOK_INVALID    {

expression_statement
  expression : call_expression
    function : identifier
    arguments : argument_list
      args[] = { string, constant, identifier }
  terminator : punct

 |- LEX_ID         TOK_INVALID    printf

 |- LEX_PUNCT      TOK_INVALID    (
 |- LEX_STRING     TOK_INVALID    "Hello () World %d %p!\n"
 |- LEX_PUNCT      TOK_INVALID    ,
 |- LEX_INT        TOK_INVALID    12345
 |- LEX_PUNCT      TOK_INVALID    ,
 |- LEX_ID         TOK_INVALID    argv
 |- LEX_PUNCT      TOK_INVALID    )

 |- LEX_PUNCT      TOK_INVALID    ;
 |- LEX_ID         TOK_INVALID    return
 |- LEX_INT        TOK_INVALID    0
 |- LEX_PUNCT      TOK_INVALID    ;
 |- LEX_PUNCT      TOK_INVALID    }
 #endif

//------------------------------------------------------------------------------

Node* fold_nodes(Node* a, Node* b, LexemeType lex, TokenType tok) {
  auto parent = a->parent;

  auto n = new Node(lex, tok, a->span_a, b->span_b);

  n->parent = nullptr;
  n->tok_prev = a->tok_prev;
  n->tok_next = b->tok_next;
  n->lex_prev = nullptr;
  n->lex_next = nullptr;
  n->tok_head = a;
  n->tok_tail = b;

  if (n->tok_prev) n->tok_prev->tok_next = n;
  if (n->tok_next) n->tok_next->tok_prev = n;

  if (parent->tok_head == a) parent->tok_head = n;
  if (parent->tok_tail == b) parent->tok_tail = n;

  a->tok_prev = nullptr;
  b->tok_next = nullptr;

  for (auto c = a; c; c = c->tok_next) c->parent = n;

  return n;
}

//------------------------------------------------------------------------------

void fold_delims(Node* a, Node* b, char ldelim, char rdelim, TokenType block_type) {

  std::stack<Node*> ldelims;

  for (Node* cursor = a; cursor; cursor = cursor->tok_next) {
    if (cursor->lexeme != LEX_PUNCT) continue;

    if (*cursor->span_a == ldelim) {
      ldelims.push(cursor);
    }
    else if (*cursor->span_a == rdelim) {
      auto node_ldelim = ldelims.top();
      auto node_rdelim = cursor;
      cursor = fold_nodes(node_ldelim, node_rdelim, LEX_INVALID, block_type);
      ldelims.pop();
    }
  }
}

//------------------------------------------------------------------------------

Node* parse_translation_unit(Node* root) {
  //Node* unit = new Node();

  for(auto cursor = root->tok_head; cursor; cursor = cursor->tok_next) {
    if (cursor->lexeme == LEX_PREPROC) {
      printf("<preproc>\n");
      continue;
    }
    else {
      /*
      using function_definition = Seq<
        Opt<attribute_specifier_sequence>,
        declaration_specifiers,
        declarator,
        function_body
      >;
      */
    }
  }

  return nullptr;
}

//------------------------------------------------------------------------------

int test_c99_peg() {
  printf("Hello World\n");

  auto text_a = text;
  auto text_b = text + strlen(text);

  auto cursor = text_a;

  Node* root = new Node(LEX_ROOT, TOK_ROOT, text_a, text_b);

  while(cursor < text_b) {
    auto t = next_token(cursor, text_b);

    if (t) {
      auto n = new Node(t.lex, TOK_INVALID, t.span_a, t.span_b);
      root->append_lex(n);

      if (!n->is_gap()) root->append_tok(n);

      cursor = t.span_b;
    }
    else {
      break;
    }
  }

  printf("//----------------------------------------\n");

  dump_tree(root, 100);

  printf("//----------------------------------------\n");

  return 0;
}

//------------------------------------------------------------------------------
