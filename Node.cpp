#include "Node.h"
#include <stdio.h>

//------------------------------------------------------------------------------

void dump_span(int max_len, const char* a, const char* b) {
  auto len = b - a;
  if (len < max_len) {
    for (int i = 0; i < len; i++) {
      putc(a[i], stdout);
    }
    for (int i = len; i < max_len; i++) {
      //putc('.', stdout);
    }
  }
  else {
    for (int i = 0; i < max_len - 3; i++) {
      auto c = a[i];
      if (c == '\n') {
        printf("\\n");
      }
      else if (c == '\t') {
        printf("\\t");
      }
      else {
        printf("%c", c);
      }
    }
    for (int i = max_len - 3; i < max_len; i++) {
      //putc('.', stdout);
    }
  }
}

//------------------------------------------------------------------------------

void dump_node(Node* n) {
  //printf("%-15s", lex_to_str(n->lexeme));
  printf("%-15s", tok_to_str(n->node_type));
  /*
  if (n->lexeme == LEX_PUNCT) {
    printf("%c", *n->span_a);
  }
  else if (n->lexeme == LEX_NEWLINE) {
  }
  else if (n->lexeme == LEX_SPACE) {
  }
  else {
    dump_span(20, n->span_a, n->span_b);
  }
  */
  printf("\n");
}

//------------------------------------------------------------------------------

void dump_gap(Node* n, int indentation) {
  /*
  if (!n) {
    printf("dump_gap got nullptr\n");
    return;
  }

  auto a = n;
  auto b = a->tok_next;

  if (!a || !b) {
    //printf("can't dump gap?\n");
    return;
  }

  while(a && a->lex_next == nullptr && a->tok_tail) {
    a = a->tok_tail;
  }

  while(b && b->lex_next == nullptr && b->tok_head) {
    b = b->tok_head;
  }

  if (!a || !b) {
    printf("Couldn't find token chain?\n");
    return;
  }


  for (auto cursor = a->lex_next; cursor != b; cursor = cursor->lex_next) {
    if (cursor == nullptr) {
      printf("something went wrong\n");
      return;
    }

    for (int i = 0; i < indentation; i++) printf("  ");
    printf("*");
    dump_node(cursor);
  }
  */
}

//------------------------------------------------------------------------------

void dump_tree(Node* head, int max_depth, int indentation) {
  if (indentation > max_depth) return;

  for (auto cursor = head; cursor; cursor = cursor->next) {

    for (int i = 0; i < indentation-1; i++) printf(" |  ");
    if (indentation) {
      if (!cursor->next || !cursor->prev) {
        printf(" |- ");
      }
      else {
        printf(" |- ");
      }
    }

    dump_node(cursor);
    if (cursor->head) {
      dump_tree(cursor->head, max_depth, indentation + 1);
    }
    //dump_gap(cursor, indentation + 1);
  }
}
