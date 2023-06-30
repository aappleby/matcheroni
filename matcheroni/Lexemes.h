#pragma once
#include <stdint.h>
#include <string>

#include "Matcheroni.h"

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
  Lexeme(LexemeType type, const char* span_a, const char* span_b) {
    this->type = type;
    this->span_a = span_a;
    this->span_b = span_b;
  }

  int len() const {
    return int(span_b - span_a);
  }

  inline static int text_count = 0;

  // called by nodeidentifier::text and the class/struct/union/enum/typedef type lookups
  std::string text() const {
    text_count++;
    return std::string(span_a, span_b);
  }

  //----------------------------------------

  bool is_bof() const {
    return type == LEX_BOF;
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

  bool is_preproc() const {
    return type == LEX_PREPROC;
  }

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
    if (c != span_b) return false;

    return true;
  }

  bool is_identifier(const char* lit = nullptr) const {
    return (type == LEX_IDENTIFIER) && (lit == nullptr || is_lit(lit));
  }

  //----------------------------------------

  LexemeType  type;
  const char* span_a;
  const char* span_b;
};

//------------------------------------------------------------------------------

inline const char* lex_to_str(const Lexeme& lex) {
  switch(lex.type) {
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

inline uint32_t lex_to_color(const Lexeme& lex) {
  switch(lex.type) {
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
    case LEX_BOF        : return 0x00FF00;
    case LEX_EOF        : return 0x0000FF;
    case LEX_LAST       : return 0xFF00FF;
  }
  return 0x0000FF;
}

//------------------------------------------------------------------------------
// Does this actually work?

struct AngleSpan;
struct BraceSpan;
struct BrackSpan;
struct ParenSpan;
struct DQuoteSpan;
struct SQuoteSpan;

using AnySpan = Oneof<AngleSpan, BraceSpan, BrackSpan, ParenSpan, DQuoteSpan, SQuoteSpan>;

template<auto ldelim, auto rdelim>
using DelimitedSpan =
Seq<
  Atom<ldelim>,
  Any<
    AnySpan,
    NotAtom<rdelim>
  >,
  Atom<rdelim>
>;

struct AngleSpan : public PatternWrapper<AngleSpan> {
  using pattern = DelimitedSpan<'<', '>'>;
};

struct BraceSpan : public PatternWrapper<BraceSpan> {
  using pattern = DelimitedSpan<'{', '}'>;
};

struct BrackSpan : public PatternWrapper<BrackSpan> {
  using pattern = DelimitedSpan<'[', ']'>;
};

struct ParenSpan : public PatternWrapper<ParenSpan> {
  using pattern = DelimitedSpan<'(', ')'>;
};

struct DQuoteSpan : public PatternWrapper<DQuoteSpan> {
  using pattern = DelimitedSpan<'"', '"'>;
};

struct SQuoteSpan : public PatternWrapper<SQuoteSpan> {
  using pattern = DelimitedSpan<'\'', '\''>;
};

inline const char* find_matching_delim(void* ctx, const char* a, const char* b) {
  return AnySpan::match(ctx, a, b);
  /*
  char ldelim = *a++;

  char rdelim = 0;
  if (ldelim == '<')  rdelim = '>';
  if (ldelim == '{')  rdelim = '}';
  if (ldelim == '[')  rdelim = ']';
  if (ldelim == '"')  rdelim = '"';
  if (ldelim == '\'') rdelim = '\'';
  if (!rdelim) return nullptr;

  while(a && *a && a < b) {
    if (*a == rdelim) return a;

    if (*a == '<' || *a == '{' || *a == '[' || *a == '"' || *a == '\'') {
      a = find_matching_delim(a, b);
      if (!a) return nullptr;
      a++;
    }
    else if (ldelim == '"' && a[0] == '\\' && a[1] == '"') {
      a += 2;
    }
    else if (ldelim == '\'' && a[0] == '\\' && a[1] == '\'') {
      a += 2;
    }
    else {
      a++;
    }
  }

  return nullptr;
  */
}

//------------------------------------------------------------------------------
