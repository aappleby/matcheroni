// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#include <stdio.h>
#include <string>
#include <vector>
#include <array>

#include "examples/utils.hpp"
#include "examples/c99_parser/SlabAlloc.hpp"
#include "examples/c99_parser/c_constants.hpp"
#include "examples/c99_parser/SST.hpp"
#include "matcheroni/Matcheroni.hpp"
using namespace matcheroni;

struct C99Parser;
struct Lexeme;
struct ParseNode;
struct SlabAlloc;
struct Token;
struct TypeScope;

//------------------------------------------------------------------------------

enum LexemeType {
  LEX_INVALID = 0,
  LEX_SPACE,
  LEX_NEWLINE,
  LEX_STRING,
  LEX_KEYWORD,
  LEX_IDENTIFIER,
  LEX_COMMENT,
  LEX_PREPROC,
  LEX_FLOAT,
  LEX_INT,
  LEX_PUNCT,
  LEX_CHAR,
  LEX_SPLICE,
  LEX_FORMFEED,
  LEX_BOF,
  LEX_EOF,
  LEX_LAST
};

//------------------------------------------------------------------------------

struct Lexeme {
  Lexeme(LexemeType type, const char* span_a, const char* span_b);

  int len() const;
  bool is_bof() const;
  bool is_eof() const;
  bool is_gap() const;

  const char* type_to_str() const;
  uint32_t type_to_color() const;
  void dump_lexeme() const;

  //----------------------------------------

  LexemeType  type;
  const char* span_a;
  const char* span_b;
};

//------------------------------------------------------------------------------

struct C99Lexer {
  C99Lexer();
  void reset();
  bool lex(const std::string& text);
  void dump_lexemes();

  std::vector<Lexeme> lexemes;
};

//------------------------------------------------------------------------------
// Tokens associate lexemes with parse nodes.
// Tokens store bookkeeping data during parsing.

struct Token {
  Token(const Lexeme* lex);

  //----------------------------------------
  // These methods REMOVE THE SPAN FROM THE NODE - that's why they're not
  // const. This is required to ensure that if an Opt<> matcher fails to match
  // a branch, when it tries to match the next branch we will always pull the
  // defunct nodes off the tokens.

  int atom_cmp(const LexemeType& b);
  int atom_cmp(const char& b);
  int atom_cmp(const char* b);

  template <int N>
  int atom_cmp(const StringParam<N>& b) {
    clear_span();
    if (int c = lex->len() - b.str_len) return c;
    for (auto i = 0; i < b.str_len; i++) {
      if (auto c = lex->span_a[i] - b.str_val[i]) return c;
    }
    return 0;
  }

  int atom_cmp(const Token* b);
  const char* unsafe_span_a();

  //----------------------------------------

  ParseNode* get_span();
  const ParseNode* get_span() const;
  void set_span(ParseNode* n);
  void clear_span();
  const char* type_to_str() const;
  uint32_t type_to_color() const;
  void dump_token() const;

  Token* step_left();
  Token* step_right();

  //----------------------------------------

  const char* debug_span_a() const;
  const char* debug_span_b() const;
  const Lexeme* get_lex_debug() const;

  bool dirty = false;

private:
  ParseNode* span;
  const Lexeme* lex;
};

//------------------------------------------------------------------------------

struct ParseNode {
  static SlabAlloc slabs;

  static void* operator new(std::size_t size);
  static void* operator new[](std::size_t size);
  static void operator delete(void*);
  static void operator delete[](void*);

  virtual ~ParseNode();

  bool in_range(const Token* a, const Token* b);
  virtual void init_node(void* ctx, Token* tok_a, Token* tok_b,
                         ParseNode* node_a, ParseNode* node_b);
  virtual void init_leaf(void* ctx, Token* tok_a, Token* tok_b);
  void attach_child(ParseNode* child);
  int node_count() const;
  ParseNode* left_neighbor();
  ParseNode* right_neighbor();

  //----------------------------------------

  template <typename P>
  bool is_a() const {
    return typeid(*this) == typeid(P);
  }

  template <typename P>
  P* child() {
    for (auto cursor = head; cursor; cursor = cursor->next) {
      if (cursor->is_a<P>()) {
        return dynamic_cast<P*>(cursor);
      }
    }
    return nullptr;
  }

  template <typename P>
  const P* child() const {
    for (auto cursor = head; cursor; cursor = cursor->next) {
      if (cursor->is_a<P>()) {
        return dynamic_cast<const P*>(cursor);
      }
    }
    return nullptr;
  }

  template <typename P>
  P* search() {
    if (this->is_a<P>()) return this->as_a<P>();
    for (auto cursor = head; cursor; cursor = cursor->next) {
      if (auto c = cursor->search<P>()) {
        return c;
      }
    }
    return nullptr;
  }

  template <typename P>
  P* as_a() {
    return dynamic_cast<P*>(this);
  }

  template <typename P>
  const P* as_a() const {
    return dynamic_cast<const P*>(this);
  }

  //----------------------------------------

  void print_class_name(int max_len = 0) const;
  void check_sanity();

  Token* tok_a();
  Token* tok_b();
  const Token* tok_a() const;
  const Token* tok_b() const;

  void dump_tree(int max_depth, int indentation);

  //----------------------------------------

  inline static int constructor_count = 0;

  int precedence = 0;

  // -2 = prefix, -1 = right-to-left, 0 = none, 1 = left-to-right, 2 = suffix
  int assoc = 0;

  ParseNode* prev = nullptr;
  ParseNode* next = nullptr;
  ParseNode* head = nullptr;
  ParseNode* tail = nullptr;

 private:
  Token* _tok_a = nullptr;  // First token, inclusivve
  Token* _tok_b = nullptr;  // Last token, inclusive
};

//------------------------------------------------------------------------------

struct ParseNodeIterator {
  ParseNodeIterator(ParseNode* cursor) : n(cursor) {}
  ParseNodeIterator& operator++() {
    n = n->next;
    return *this;
  }
  bool operator!=(ParseNodeIterator& b) const { return n != b.n; }
  ParseNode* operator*() const { return n; }
  ParseNode* n;
};

inline ParseNodeIterator begin(ParseNode* parent) {
  return ParseNodeIterator(parent->head);
}

inline ParseNodeIterator end(ParseNode* parent) {
  return ParseNodeIterator(nullptr);
}

struct ConstParseNodeIterator {
  ConstParseNodeIterator(const ParseNode* cursor) : n(cursor) {}
  ConstParseNodeIterator& operator++() {
    n = n->next;
    return *this;
  }
  bool operator!=(const ConstParseNodeIterator& b) const { return n != b.n; }
  const ParseNode* operator*() const { return n; }
  const ParseNode* n;
};

inline ConstParseNodeIterator begin(const ParseNode* parent) {
  return ConstParseNodeIterator(parent->head);
}

inline ConstParseNodeIterator end(const ParseNode* parent) {
  return ConstParseNodeIterator(nullptr);
}

//------------------------------------------------------------------------------

struct TypeScope {

  using token_list = std::vector<const Token*>;

  void clear();
  bool has_type(void* ctx, Token* a, Token* b, token_list& types);
  void add_type(Token* a, token_list& types);

  bool has_class_type  (void* ctx, Token* a, Token* b);
  bool has_struct_type (void* ctx, Token* a, Token* b);
  bool has_union_type  (void* ctx, Token* a, Token* b);
  bool has_enum_type   (void* ctx, Token* a, Token* b);
  bool has_typedef_type(void* ctx, Token* a, Token* b);

  void add_class_type  (Token* a);
  void add_struct_type (Token* a);
  void add_union_type  (Token* a);
  void add_enum_type   (Token* a);
  void add_typedef_type(Token* a);

  TypeScope* parent;
  token_list class_types;
  token_list struct_types;
  token_list union_types;
  token_list enum_types;
  token_list typedef_types;
};

//------------------------------------------------------------------------------

class C99Parser {
 public:
  C99Parser();

  void reset();
  bool parse(std::vector<Lexeme>& lexemes);

  Token* match_builtin_type_base(Token* a, Token* b);
  Token* match_builtin_type_prefix(Token* a, Token* b);
  Token* match_builtin_type_suffix(Token* a, Token* b);

  Token* match_class_type(Token* a, Token* b);
  Token* match_struct_type(Token* a, Token* b);
  Token* match_union_type(Token* a, Token* b);
  Token* match_enum_type(Token* a, Token* b);
  Token* match_typedef_type(Token* a, Token* b);

  void add_class_type(Token* a);
  void add_struct_type(Token* a);
  void add_union_type(Token* a);
  void add_enum_type(Token* a);
  void add_typedef_type(Token* a);

  void push_scope();
  void pop_scope();

  void append_node(ParseNode* node);
  void enclose_nodes(ParseNode* start, ParseNode* node);

  void dump_stats();
  void dump_lexemes();
  void dump_tokens();

  int atom_cmp(Token* a, const LexemeType& b);
  int atom_cmp(Token* a, const char& b);
  int atom_cmp(Token* a, const char* b);
  int atom_cmp(Token* a, const Token* b);
  void atom_rewind(Token* a, Token* b);

  template<int N>
  inline int atom_cmp(Token* a, const StringParam<N>& b) {
    DCHECK(a == global_cursor);
    auto result = a->atom_cmp(b);
    if (result == 0) global_cursor++;
    return result;
  }

  //----------------------------------------

  std::vector<Token> tokens;
  TypeScope* type_scope;

  ParseNode* head = nullptr;
  ParseNode* tail = nullptr;
  ParseNode* root = nullptr;

  Token* global_cursor;
  inline static int rewind_count = 0;
  inline static int didnt_rewind = 0;
};

//------------------------------------------------------------------------------

template<>
inline int matcheroni::atom_cmp(void* ctx, Token* a, LexemeType b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

template<>
inline int matcheroni::atom_cmp(void* ctx, Token* a, char b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

template<>
inline int matcheroni::atom_cmp(void* ctx, Token* a, const char* b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

template<int N>
inline int atom_cmp(void* ctx, Token* a, const StringParam<N>& b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

template<>
inline int matcheroni::atom_cmp(void* ctx, Token* a, const Token* b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

template<>
inline void matcheroni::atom_rewind(void* ctx, Token* a, Token* b) {
  ((C99Parser*)ctx)->atom_rewind(a, b);
}

//------------------------------------------------------------------------------
// Sorted string table matcher thing.

template<const auto& table>
struct SST;

template<typename T, auto N, const std::array<T, N>& table>
struct SST<table> {

  constexpr inline static int top_bit(int x) {
    for (int b = 31; b >= 0; b--) {
      if (x & (1 << b)) return (1 << b);
    }
    return 0;
  }

  constexpr static bool contains(const char* s) {
    for (auto t : table) {
      if (__builtin_strcmp(t, s) == 0) return true;
    }
    return false;
  }

  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    int bit = top_bit(N);
    int index = 0;

    // I'm not actually sure if 8 is the best tradeoff but it seems OK
    if (N > 8) {
      // Binary search for large tables
      while(1) {
        auto new_index = index | bit;
        if (new_index < N) {
          auto lit = table[new_index];
          auto c = atom_cmp(ctx, a, lit);
          if (c == 0) return a + 1;
          if (c > 0) index = new_index;
        }
        if (bit == 0) return nullptr;
        bit >>= 1;
      }
    }
    else {
      // Linear scan for small tables
      for (auto lit : table) {
        if (atom_cmp(ctx, a, lit) == 0) return a + 1;
      }
    }

    return nullptr;
  }
};

//------------------------------------------------------------------------------
