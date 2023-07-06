#pragma once

#include <vector>
#include <string>
#include "Lexeme.hpp"

//------------------------------------------------------------------------------

struct C99Lexer {
  C99Lexer();
  void reset();
  bool lex(const std::string& text);
  void dump_lexemes();

  std::vector<Lexeme> lexemes;
};

//------------------------------------------------------------------------------
