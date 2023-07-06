#pragma once

#include <string>  // for string
#include <vector>  // for vector

#include "Lexeme.hpp"  // for Lexeme, LexemeType
#include "Token.hpp"

struct ParseNode;
struct TypeScope;

//------------------------------------------------------------------------------

class C99Parser {
 public:
  C99Parser();

  void reset();
  void load(const std::string& path);
  void lex();
  ParseNode* parse();

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

  /*
  template<int N>
  int atom_cmp(Token* a, const StringParam<N>& b);
    DCHECK(a == global_cursor);
    auto result = a->atom_cmp(b);
    if (result == 0) global_cursor++;
    return result;
  }
  */

  //----------------------------------------

  std::string text;
  std::vector<Lexeme> lexemes;
  std::vector<Token> tokens;

  ParseNode* head = nullptr;
  ParseNode* tail = nullptr;

  int file_pass = 0;
  int file_fail = 0;
  int file_skip = 0;
  int file_bytes = 0;
  int file_lines = 0;

  double io_accum = 0;
  double lex_accum = 0;
  double parse_accum = 0;
  double cleanup_accum = 0;

  TypeScope* type_scope;

  Token* global_cursor;
  inline static int rewind_count = 0;
  inline static int didnt_rewind = 0;
};

//------------------------------------------------------------------------------
