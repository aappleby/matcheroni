// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#include <vector>
#include <string>
#include "c99/Lexeme.hpp"

//------------------------------------------------------------------------------

struct C99Lexer {
  C99Lexer();
  void reset();
  bool lex(const std::string& text);
  void dump_lexemes();

  std::vector<Lexeme> lexemes;
};

//------------------------------------------------------------------------------
