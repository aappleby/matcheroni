#pragma once
#include <stdint.h>
#include <string>

#include "Matcheroni.h"

#ifdef MATCHERONI_USE_NAMESPACE
using namespace matcheroni;
#endif

//------------------------------------------------------------------------------

struct cspan {
  const char* a;
  const char* b;
};

inline int atom_cmp(const cspan& s, const char* b) {
  return cmp_span_lit(s.a, s.b, b);
}

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

  //----------------------------------------------------------------------------

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

  int len() const {
    return span_b - span_a;
  }

  //----------------------------------------------------------------------------

  const char* type_to_str() const {
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

  uint32_t type_to_color() const {
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
      case LEX_BOF        : return 0x00FF00;
      case LEX_EOF        : return 0x0000FF;
      case LEX_LAST       : return 0xFF00FF;
    }
    return 0x0000FF;
  }

  //----------------------------------------

  void dump_lexeme() const {
    if (type == LEX_BOF) {
      printf("{<bof>     }");
      return;
    }
    if (type == LEX_EOF) {
      printf("{<eof>     }");
      return;
    }

    int len = span_b - span_a;
    const int span_len = 16;
    if (len > span_len) len = span_len;
    printf("{");
    for (int i = 0; i < len; i++) {
      auto c = span_a[i];
      if (c == '\n' || c == '\t' || c == '\r') {
        putc('@', stdout);
      }
      else {
        putc(span_a[i], stdout);
      }
    }
    for (int i = len; i < span_len; i++) {
      printf(" ");
    }
    printf("}");
  }

  //----------------------------------------

  LexemeType  type;
  const char* span_a;
  const char* span_b;
};

//------------------------------------------------------------------------------
// Does this actually work?

/*
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
}
*/

//------------------------------------------------------------------------------
