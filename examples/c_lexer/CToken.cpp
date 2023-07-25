// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c_lexer/CToken.hpp"

#include <stdio.h>

using namespace matcheroni;

//------------------------------------------------------------------------------

CToken::CToken(LexemeType type, TextSpan text) {
  this->type = type;
  this->text.begin = text.begin;
  this->text.end = text.end;
}

//----------------------------------------------------------------------------

bool CToken::is_bof() const {
  return type == LEX_BOF;
}

bool CToken::is_eof() const {
  return type == LEX_EOF;
}

bool CToken::is_gap() const {
  switch(type) {
    case LEX_NEWLINE:
    case LEX_SPACE:
    case LEX_COMMENT:
    case LEX_SPLICE:
    case LEX_FORMFEED:
      return true;
    default:
      return false;
  }
}

//----------------------------------------------------------------------------

const char* CToken::type_to_str() const {
  switch(type) {
    case LEX_INVALID    : return "LEX_INVALID";
    case LEX_SPACE      : return "LEX_SPACE";
    case LEX_NEWLINE    : return "LEX_NEWLINE";
    case LEX_STRING     : return "LEX_STRING";
    case LEX_KEYWORD    : return "LEX_KEYWORD";
    case LEX_IDENTIFIER : return "LEX_IDENTIFIER";
    case LEX_COMMENT    : return "LEX_COMMENT";
    case LEX_PREPROC    : return "LEX_PREPROC";
    case LEX_FLOAT      : return "LEX_FLOAT";
    case LEX_INT        : return "LEX_INT";
    case LEX_PUNCT      : return "LEX_PUNCT";
    case LEX_CHAR       : return "LEX_CHAR";
    case LEX_SPLICE     : return "LEX_SPLICE";
    case LEX_FORMFEED   : return "LEX_FORMFEED";
    case LEX_BOF        : return "LEX_BOF";
    case LEX_EOF        : return "LEX_EOF";
    case LEX_LAST       : return "<lex last>";
  }
  return "<invalid>";
}

//----------------------------------------------------------------------------

uint32_t CToken::type_to_color() const {
  switch(type) {
    case LEX_INVALID    : return 0x0000FF;
    case LEX_SPACE      : return 0x804040;
    case LEX_NEWLINE    : return 0x404080;
    case LEX_STRING     : return 0x4488AA;
    case LEX_KEYWORD    : return 0x0088FF;
    case LEX_IDENTIFIER : return 0xCCCC40;
    case LEX_COMMENT    : return 0x66AA66;
    case LEX_PREPROC    : return 0xCC88CC;
    case LEX_FLOAT      : return 0xFF88AA;
    case LEX_INT        : return 0xFF8888;
    case LEX_PUNCT      : return 0x808080;
    case LEX_CHAR       : return 0x44DDDD;
    case LEX_SPLICE     : return 0x00CCFF;
    case LEX_FORMFEED   : return 0xFF00FF;
    case LEX_BOF        : return 0x80FF80;
    case LEX_EOF        : return 0x8080FF;
    case LEX_LAST       : return 0xFF00FF;
  }
  return 0xFF00FF;
}

//----------------------------------------------------------------------------

void CToken::dump() const {
  const int span_len = 20;
  std::string dump = "";

  if (type == LEX_BOF) dump = "<bof>";
  if (type == LEX_EOF) dump = "<eof>";

  for (auto c = text.begin; c < text.end; c++) {
    if      (*c == '\n') dump += "\\n";
    else if (*c == '\t') dump += "\\t";
    else if (*c == '\r') dump += "\\r";
    else                 dump += *c;
    if (dump.size() >= span_len) break;
  }

  dump = '`' + dump + '`';
  if (dump.size() > span_len) {
    dump.resize(span_len - 4);
    dump = dump + "...`";
  }
  while (dump.size() < span_len) dump += " ";

  utils::set_color(type_to_color());
  printf("%-14.14s ", type_to_str());
  utils::set_color(0);
  printf("%s", dump.c_str());
  utils::set_color(0);
}

//----------------------------------------------------------------------------
