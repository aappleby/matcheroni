// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include <stdint.h>

#include "examples/c_parser/c_constants.hpp"
#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

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

struct CToken {
  CToken(LexemeType type, matcheroni::TextSpan text);

  const char* text_head() const { return text.a; }
  const char* text_tail() const { return text.b; }

  bool is_bof() const;
  bool is_eof() const;
  bool is_gap() const;

  const char* type_to_str() const;
  uint32_t type_to_color() const;
  void dump() const;

  //----------------------------------------

  LexemeType type;
  matcheroni::TextSpan text;
};

//------------------------------------------------------------------------------
