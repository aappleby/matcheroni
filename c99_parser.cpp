#include "Matcheroni.h"
#include <assert.h>
#include <stdio.h>
#include <memory.h>
#include "c_lexer.h"
#include "Node.h"

#include <stack>

using namespace matcheroni;

const char* text = "a[1+2]";

#if 0
const char* text = R"(
#include <stdio.h>

// This is a comment.

int main(int argc, char** argv) {
  // Another comment
  printf("Hello () World %d %p!\n", (1 + (2 + (4 + darp[77*clonk]))), argv);
  return 0;
}

/*
Third comment
*/

)";
#endif

#if 0




emit_map emit_sym_map = {
  { alias_sym_field_identifier,         &MtCursor::emit_text },
  { alias_sym_namespace_identifier,     &MtCursor::emit_text },
  { alias_sym_type_identifier,          &MtCursor::emit_sym_type_identifier },
  { sym_access_specifier,               &MtCursor::comment_out },
  { sym_argument_list,                  &MtCursor::emit_children },
  { sym_array_declarator,               &MtCursor::emit_children },
  { sym_assignment_expression,          &MtCursor::emit_sym_assignment_expression },
  { sym_binary_expression,              &MtCursor::emit_children },
  { sym_break_statement,                &MtCursor::skip_over }, // Verilog doesn't use "break"
  { sym_call_expression,                &MtCursor::emit_sym_call_expression },
  { sym_case_statement,                 &MtCursor::emit_sym_case_statement },
  { sym_char_literal,                   &MtCursor::emit_text },
  { sym_class_specifier,                &MtCursor::emit_sym_class_specifier },
  { sym_comment,                        &MtCursor::emit_sym_comment },
  { sym_compound_statement,             &MtCursor::emit_sym_compound_statement },
  { sym_condition_clause,               &MtCursor::emit_children },
  { sym_conditional_expression,         &MtCursor::emit_children},
  { sym_declaration,                    &MtCursor::emit_sym_declaration },
  { sym_declaration_list,               &MtCursor::emit_sym_declaration_list },
  { sym_enum_specifier,                 &MtCursor::emit_sym_enum_specifier },
  { sym_enumerator_list,                &MtCursor::emit_children },
  { sym_enumerator,                     &MtCursor::emit_children },
  { sym_expression_statement,           &MtCursor::emit_sym_expression_statement },
  { sym_field_declaration,              &MtCursor::emit_sym_field_declaration },
  { sym_field_declaration_list,         &MtCursor::emit_sym_field_declaration_list },
  { sym_field_expression,               &MtCursor::emit_sym_field_expression },
  { sym_for_statement,                  &MtCursor::emit_sym_for_statement },
  { sym_function_declarator,            &MtCursor::emit_children },
  { sym_function_definition,            &MtCursor::emit_sym_function_definition },
  { sym_identifier,                     &MtCursor::emit_sym_identifier },
  { sym_if_statement,                   &MtCursor::emit_sym_if_statement },
  { sym_init_declarator,                &MtCursor::emit_children },
  { sym_initializer_list,               &MtCursor::emit_sym_initializer_list },
  { sym_namespace_definition,           &MtCursor::emit_sym_namespace_definition },
  { sym_nullptr,                        &MtCursor::emit_sym_nullptr },
  { sym_number_literal,                 &MtCursor::emit_sym_number_literal },
  { sym_parameter_declaration,          &MtCursor::emit_children },
  { sym_parameter_list,                 &MtCursor::emit_children },
  { sym_parenthesized_expression,       &MtCursor::emit_children },
  { sym_pointer_declarator,             &MtCursor::emit_sym_pointer_declarator },

  { sym_preproc_arg,                    &MtCursor::emit_sym_preproc_arg },
  { sym_preproc_call,                   &MtCursor::skip_over },
  { sym_preproc_def,                    &MtCursor::emit_sym_preproc_def },
  { sym_preproc_else,                   &MtCursor::skip_over },
  { sym_preproc_if,                     &MtCursor::skip_over },
  { sym_preproc_ifdef,                  &MtCursor::emit_sym_preproc_ifdef },
  { sym_preproc_include,                &MtCursor::emit_sym_preproc_include },

  { sym_primitive_type,                 &MtCursor::emit_text },
  { sym_qualified_identifier,           &MtCursor::emit_sym_qualified_identifier },
  { sym_return_statement,               &MtCursor::emit_sym_return_statement },
  { sym_sized_type_specifier,           &MtCursor::emit_sym_sized_type_specifier },
  { sym_storage_class_specifier,        &MtCursor::comment_out },
  { sym_string_literal,                 &MtCursor::emit_text },
  { sym_struct_specifier,               &MtCursor::emit_sym_struct_specifier },
  { sym_subscript_expression,           &MtCursor::emit_children },
  { sym_switch_statement,               &MtCursor::emit_sym_switch_statement },
  { sym_template_argument_list,         &MtCursor::emit_sym_template_argument_list },
  { sym_template_declaration,           &MtCursor::emit_sym_template_declaration },
  { sym_template_type,                  &MtCursor::emit_sym_template_type },
  { sym_translation_unit,               &MtCursor::emit_sym_translation_unit },
  { sym_type_definition,                &MtCursor::emit_children },
  { sym_type_descriptor,                &MtCursor::emit_children },
  { sym_type_qualifier,                 &MtCursor::comment_out },
  { sym_unary_expression,               &MtCursor::emit_children },
  { sym_update_expression,              &MtCursor::emit_sym_update_expression },
  { sym_using_declaration,              &MtCursor::emit_sym_using_declaration },
};
#endif

//------------------------------------------------------------------------------

void dump_span(int max_len, const char* a, const char* b) {
  auto len = b - a;
  if (len < max_len) {
    for (int i = 0; i < len; i++) {
      putc(a[i], stdout);
    }
    for (int i = len; i < max_len; i++) {
      putc('.', stdout);
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
      putc('.', stdout);
    }
  }

}

void dump_node(Node* n) {
  printf("%-15s", lex_to_str(n->lexeme));
  printf("%-15s", tok_to_str(n->token));
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
  printf("\n");
}

//------------------------------------------------------------------------------

void skip_gaps(Node*& cursor) {
 while(cursor && cursor->is_gap()) {
    cursor = cursor->lex_next;
  }
}

void fold_gaps(Node* root) {
  Node* cursor = root->lex_head;

  while(cursor) {
    skip_gaps(cursor);
    if (cursor) {
      root->append_tok(cursor);
      cursor = cursor->lex_next;
    }
  }
}

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

void dump_gap(Node* n, int indentation) {
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
}

void dump_tree(Node* head, int max_depth = 1000, int indentation = 0) {
  if (indentation > max_depth) return;

  for (auto cursor = head; cursor; cursor = cursor->tok_next) {

    for (int i = 0; i < indentation-1; i++) printf(" |  ");
    if (indentation) {
      if (!cursor->tok_next || !cursor->tok_prev) {
        printf(" |- ");
      }
      else {
        printf(" |- ");
      }
    }

    dump_node(cursor);
    if (cursor->tok_head) {
      dump_tree(cursor->tok_head, max_depth, indentation + 1);
    }
    //dump_gap(cursor, indentation + 1);
  }
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
      cursor = t.span_b;
    }
    else {
      break;
    }
  }

  printf("//----------------------------------------\n");

  fold_gaps(root);
  fold_delims(root->tok_head, root->tok_tail, '[', ']', BLOCK_BRACK);
  /*
  fold_delims(&tokens, '(', ')', BLOCK_PAREN);
  fold_delims(&tokens, '{', '}', BLOCK_BRACE);
  */
  dump_tree(root, 100);

  printf("//----------------------------------------\n");

  return 0;
}

//------------------------------------------------------------------------------
