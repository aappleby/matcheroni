#pragma once

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
  operator bool() const {
    return lexeme != LEX_INVALID;
  }

  bool is_gap() const {
    switch(lexeme) {
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
    switch(lexeme) {
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

  LexemeType    lexeme = LEX_INVALID;
  const char*   span_a = nullptr;
  const char*   span_b = nullptr;
};

//------------------------------------------------------------------------------

namespace parseroni {

inline bool match_str(const char* a, const char* b, const char* lit) {
  auto c = a;
  for (;c < b && (*c == *lit) && *lit; c++, lit++);
  return *lit ? false : true;
}

inline bool eq(const Lexeme* l, char c) {
  return *l->span_a == c;
}

inline bool eq(const Lexeme* l, const char* lit) {
  return match_str(l->span_a, l->span_b, lit);
}

template <auto... rest>
struct Atom;

template <auto C, auto... rest>
struct Atom<C, rest...> {
  using atom = Lexeme;
  using pattern = decltype(C);

  static const atom* match(const atom* a, const atom* b, void* ctx) {
    if (!a || a == b) return nullptr;
    if (*a->span_a == C) {
      return a + 1;
    } else {
      return Atom<rest...>::match(a, b, ctx);;
    }
  }
};

template <auto C>
struct Atom<C> {
  using atom = decltype(C);

  static const Lexeme* match(const atom* a, const atom* b, void* ctx) {
    if (!a || a == b) return nullptr;
    if (*a->span_a == C) {
      return a + 1;
    } else {
      return nullptr;
    }
  }
};




};

//------------------------------------------------------------------------------
