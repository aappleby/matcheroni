// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include <stdint.h>

#include "matcheroni/Matcheroni.hpp"

using cspan = matcheroni::Span<const char>;

//------------------------------------------------------------------------------

struct Lexeme {
  Lexeme(LexemeType type, cspan s);

  int len() const;
  bool is_bof() const;
  bool is_eof() const;
  bool is_gap() const;

  const char* type_to_str() const;
  uint32_t type_to_color() const;
  void dump_lexeme() const;

  //----------------------------------------

  LexemeType type;
  cspan span;
};

//------------------------------------------------------------------------------
