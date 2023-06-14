#pragma once
#include <assert.h>
#include <stdint.h>

//------------------------------------------------------------------------------

enum LexemeType {
  LEX_INVALID = 0,
  LEX_SPACE,
  LEX_NEWLINE,
  LEX_STRING,
  LEX_IDENTIFIER,
  LEX_COMMENT,
  LEX_PREPROC,
  LEX_FLOAT,
  LEX_INT,
  LEX_PUNCT,
  LEX_CHAR,
  LEX_SPLICE,
  LEX_FORMFEED,
  LEX_EOF,
};

//------------------------------------------------------------------------------

struct Lexeme {
  Lexeme(LexemeType type, const char* span_a, const char* span_b) {
    assert((type == LEX_INVALID) ^ (span_a != nullptr));
    assert((type == LEX_INVALID) ^ (span_b != nullptr));
    this->type = type;
    this->span_a = span_a;
    this->span_b = span_b;
  }

  const char* match(const char* lit) const {
    auto c = span_a;
    for (; c < span_b && (*c == *lit) && *lit; c++, lit++);
    if (*lit == 0) {
      return c;
    }
    else {
      return nullptr;
    }
  }

  bool is_valid() const {
    return type != LEX_INVALID;
  }

  bool is_eof() const {
    return type == LEX_EOF;
  }

  bool is_gap() const {
    switch(type) {
      case LEX_NEWLINE:
      case LEX_SPACE:
      case LEX_COMMENT:
      case LEX_SPLICE:
      case LEX_FORMFEED:
        return true;
    }
    return false;
  }

  const char* str() const {
    switch(type) {
      case LEX_INVALID:    return "LEX_INVALID";
      case LEX_SPACE:      return "LEX_SPACE";
      case LEX_NEWLINE:    return "LEX_NEWLINE";
      case LEX_STRING:     return "LEX_STRING";
      case LEX_IDENTIFIER: return "LEX_IDENTIFIER";
      case LEX_COMMENT:    return "LEX_COMMENT";
      case LEX_PREPROC:    return "LEX_PREPROC";
      case LEX_FLOAT:      return "LEX_FLOAT";
      case LEX_INT:        return "LEX_INT";
      case LEX_PUNCT:      return "LEX_PUNCT";
      case LEX_CHAR:       return "LEX_CHAR";
      case LEX_SPLICE:     return "LEX_SPLICE";
      case LEX_FORMFEED:   return "LEX_FORMFEED";
      case LEX_EOF:        return "LEX_EOF";
    }
    return "<invalid>";
  }

  int len() const {
    return int(span_b - span_a);
  }

  uint32_t color() const {
    switch(type) {
      case LEX_INVALID    : return 0x0000FF;
      case LEX_SPACE      : return 0x804040;
      case LEX_NEWLINE    : return 0x404080;
      case LEX_STRING     : return 0x4488AA;
      case LEX_IDENTIFIER : return 0xCCCC40;
      case LEX_COMMENT    : return 0x66AA66;
      case LEX_PREPROC    : return 0xCC88CC;
      case LEX_FLOAT      : return 0xFF88AA;
      case LEX_INT        : return 0xFF8888;
      case LEX_PUNCT      : return 0x808080;
      case LEX_CHAR       : return 0x44DDDD;
      case LEX_SPLICE     : return 0x00CCFF;
      case LEX_FORMFEED   : return 0xFF00FF;
      case LEX_EOF        : return 0xFF00FF;
    }
    return 0x0000FF;
  }

  //----------------------------------------

  bool is_punct() const {
    return (type == LEX_PUNCT);
  }

  bool is_punct(char p) const {
    return (type == LEX_PUNCT) && (*span_a == p);
  }

  bool is_lit(const char* lit) const {
    auto c = span_a;
    for (;c < span_b && (*c == *lit) && *lit; c++, lit++);

    if (*lit) return false;

    if ((*c >= 'a' && *c <= 'z') ||
        (*c >= 'A' && *c <= 'Z') ||
        (*c >= '0' && *c <= '9') ||
        (*c == '_')) {
      return false;
    }

    return true;
  }

  bool is_identifier(const char* lit = nullptr) const {
    return (type == LEX_IDENTIFIER) && (lit == nullptr || is_lit(lit));
  }

  bool is_constant() const {
    return (type == LEX_FLOAT) || (type == LEX_INT) || (type == LEX_CHAR) || (type == LEX_STRING);
  }

  //----------------------------------------

  static Lexeme invalid() { return Lexeme(LEX_INVALID, nullptr, nullptr); }

  LexemeType  type;
  const char* span_a;
  const char* span_b;
};

//------------------------------------------------------------------------------
