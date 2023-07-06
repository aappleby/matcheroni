// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

inline static int indent_depth = 0;

//------------------------------------------------------------------------------

template<typename NodeType, typename atom>
void print_trace_start(atom* a) {
#ifdef MATCHERONI_ENABLE_TRACE
  static constexpr int context_len = 60;
  printf("[");
  print_escaped(a->get_lex_debug()->span_a, context_len, 0x404040);
  printf("] ");
  for (int i = 0; i < indent_depth; i++) printf("|   ");
  print_class_name<NodeType>();
  printf("?\n");
  indent_depth += 1;
#endif
}

//------------------------------------------------------------------------------

template<typename NodeType, typename atom>
void print_trace_end(atom* a, atom* end) {
#ifdef MATCHERONI_ENABLE_TRACE
  static constexpr int context_len = 60;
  indent_depth -= 1;
  printf("[");
  if (end) {
    int match_len = end->get_lex_debug()->span_a - a->get_lex_debug()->span_a;
    if (match_len > context_len) match_len = context_len;
    int left_len = context_len - match_len;
    print_escaped(a->get_lex_debug()->span_a, match_len,  0x60C000);
    print_escaped(end->get_lex_debug()->span_a, left_len, 0x404040);
  }
  else {
    print_escaped(a->get_lex_debug()->span_a, context_len, 0x2020A0);
  }
  printf("] ");
  for (int i = 0; i < indent_depth; i++) printf("|   ");
  print_class_name<NodeType>();
  printf(end ? " OK\n" : " XXX\n");

#ifdef EXTRA_DEBUG
  if (end) {
    printf("\n");
    print_class_name<NodeType>();
    printf("\n");

    for (auto c = a; c < end; c++) {
      c->dump_token();
    }
    printf("\n");
  }
#endif

#endif
}

//------------------------------------------------------------------------------

template<typename NodeType>
struct Trace {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    print_trace_start<NodeType, atom>(a);
    auto end = NodeType::match(ctx, a, b);
    print_trace_end<NodeType, atom>(a, end);
    return end;
  }
};

//------------------------------------------------------------------------------
