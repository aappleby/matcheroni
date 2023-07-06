#pragma once

#include <string>  // for string
#include <vector>  // for vector

#include "Lexeme.hpp"  // for Lexeme, LexemeType
#include "Token.hpp"
#include "utils.hpp"

struct ParseNode;
struct TypeScope;

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
