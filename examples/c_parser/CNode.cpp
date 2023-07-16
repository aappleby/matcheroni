// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c_parser/CNode.hpp"

//------------------------------------------------------------------------------

#if 0
void ParseNode::dump_tree(int max_depth, int indentation) {
  const ParseNode* n = this;
  if (max_depth && indentation == max_depth) return;

  printf("%p {%p-%p} ", n, n->tok_a(), n->tok_b());

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
}
#endif

//------------------------------------------------------------------------------
