#pragma once
#include <assert.h>
#include <stdint.h>
#include <string>

#include "Matcheroni.h"

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

  std::string text() const {
    return std::string(span_a, span_b);
  }

  //----------------------------------------

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
