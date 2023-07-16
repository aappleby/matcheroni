// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#include <vector>
#include <string>
#include "examples/c_parser/CToken.hpp"

//------------------------------------------------------------------------------

struct CLexer {
  CLexer();
  void reset();
  bool lex(const std::string& text);
  void dump_lexemes();

  std::vector<CToken> lexemes;
};

//------------------------------------------------------------------------------
