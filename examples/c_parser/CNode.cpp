// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c_parser/CNode.hpp"

using namespace matcheroni;

//------------------------------------------------------------------------------

inline void print_bar2(int depth, TextSpan s, const char* val, const char* suffix) {
  set_color(0);
  printf("{");
  set_color(0xAAFFAA);
  print_flat(s, 40);
  set_color(0);
  printf("}");

  set_color(0x404040);
  printf(depth == 0 ? " *" : "  ");
  for (int i = 0; i < depth; i++) {
    printf(i == depth - 1 ? "|-" : "| ");
  }

  set_color(0xFFAAAA);
  printf("%s %s", val, suffix);
  printf("\n");

  set_color(0);
}

inline void print_tree2(const CNode* node, int depth = 0) {
  TextSpan text(node->span.a->a, node->span.b->a);

  print_bar2(depth, text, node->match_name, "");
  for (auto c = node->child_head(); c; c = c->node_next()) {
    print_tree2(c, depth + 1);
  }
}


void CNode::dump_tree(int max_depth, int indentation) {
  print_tree2(this);
  /*
  const CNode* n = this;
  if (max_depth && indentation == max_depth) return;

  printf("%s %p {%p-%p} ", n->match_name, n, n->span.a, n->span.b);
  printf("\n");
  */

  /*
  printf("{%-40.40s}", escape_span(n).c_str());

  for (int i = 0; i < indentation; i++) printf(" | ");

  if (n->precedence) {
    printf("[%02d %2d] ",
      n->precedence,
      n->assoc
      //n->assoc > 0 ? '>' : n->assoc < 0 ? '<' : '-'
    );
  }
  else {
    printf("[-O-] ");
  }

  if (n->tok_a()) set_color(n->tok_a()->type_to_color());
  //if (!field.empty()) printf("%-10.10s : ", field.c_str());

  n->print_class_name(20);
  set_color(0);
  printf("\n");

  for (auto c = n->head; c; c = c->next) {
    c->dump_tree(max_depth, indentation + 1);
  }
  */
}

//------------------------------------------------------------------------------
