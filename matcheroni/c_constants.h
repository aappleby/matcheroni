#pragma once
#include <array>
#include <string.h>
#include "Matcheroni.h"

#ifdef MATCHERONI_USE_NAMESPACE
using namespace matcheroni;
#endif

//------------------------------------------------------------------------------
// Sorted string table matcher thing.

template<const auto& table>
struct SST;

template<typename T, auto N, const std::array<T, N>& table>
struct SST<table> {

  constexpr inline static int top_bit(int x) {
    for (int b = 31; b >= 0; b--) {
      if (x & (1 << b)) return (1 << b);
    }
    return 0;
  }

  static const char* match(const char* a, const char* b) {
    int bit = top_bit(N);
    int index = 0;

    // I'm not actually sure if 8 is the best tradeoff but it seems OK
    if (N > 8) {
      // Binary search for large tables
      while(1) {
        auto new_index = index | bit;
        if (new_index < N) {
          auto lit = table[new_index];
          auto c = cmp_span_lit(a, b, lit);
          if (c == 0) return lit;
          if (c > 0) index = new_index;
        }
        if (bit == 0) return nullptr;
        bit >>= 1;
      }
    }
    else {
      // Linear scan for small tables
      for (auto lit : table) {
        if (cmp_span_lit(a, b, lit) == 0) return a + 1;
      }
    }

    return nullptr;
  }
};

//------------------------------------------------------------------------------
// MUST BE SORTED CASE-SENSITIVE

constexpr std::array c99_keywords = {
  "_Bool",
  "_Complex",
  "_Imaginary",
  "auto",
  "break",
  "case",
  "char",
  "const",
  "continue",
  "default",
  "do",
  "double",
  "else",
  "enum",
  "extern",
  "float",
  "for",
  "goto",
  "if",
  "inline",
  "int",
  "long",
  "register",
  "restrict",
  "return",
  "short",
  "signed",
  "sizeof",
  "static",
  "struct",
  "switch",
  "typedef",
  "union",
  "unsigned",
  "void",
  "volatile",
  "while",
};

//------------------------------------------------------------------------------
// MUST BE SORTED CASE-SENSITIVE

constexpr std::array builtin_type_base = {
  //"FILE", // used in fprintf.c torture test
  "_Bool",
  "_Complex", // yes this is both a prefix and a type :P
  "_Decimal128",
  "_Decimal32",
  "_Decimal64",
  "__INT16_TYPE__",
  "__INT32_TYPE__",
  "__INT64_TYPE__",
  "__INT8_TYPE__",
  "__INTMAX_TYPE__",
  "__INTPTR_TYPE__",
  "__INT_FAST16_TYPE__",
  "__INT_FAST32_TYPE__",
  "__INT_FAST64_TYPE__",
  "__INT_FAST8_TYPE__",
  "__INT_LEAST16_TYPE__",
  "__INT_LEAST32_TYPE__",
  "__INT_LEAST64_TYPE__",
  "__INT_LEAST8_TYPE__",
  "__PTRDIFF_TYPE__",
  "__SIG_ATOMIC_TYPE__",
  "__SIZE_TYPE__",
  "__UINT16_TYPE__",
  "__UINT32_TYPE__",
  "__UINT64_TYPE__",
  "__UINT8_TYPE__",
  "__UINTMAX_TYPE__",
  "__UINTPTR_TYPE__",
  "__UINT_FAST16_TYPE__",
  "__UINT_FAST32_TYPE__",
  "__UINT_FAST64_TYPE__",
  "__UINT_FAST8_TYPE__",
  "__UINT_LEAST16_TYPE__",
  "__UINT_LEAST32_TYPE__",
  "__UINT_LEAST64_TYPE__",
  "__UINT_LEAST8_TYPE__",
  "__WCHAR_TYPE__",
  "__WINT_TYPE__",
  "__builtin_va_list",
  "__imag__",
  "__int128",
  "__label__",
  "__real__",
  "bool",
  "char",
  "double",
  "float",
  "int",
  "int16_t",
  "int32_t",
  "int64_t",
  "int8_t",
  "long",
  "ptrdiff_t",
  "short",
  "signed",
  "size_t", // used in fputs-lib.c torture test
  "uint16_t",
  "uint32_t",
  "uint64_t",
  "uint8_t",
  "unsigned",
  //"va_list", // technically part of the c library, but it shows up in stdarg test files
  "void",
  "wchar_t",
};

// MUST BE SORTED CASE-SENSITIVE
constexpr std::array builtin_type_prefix = {
  "_Complex",
  "__complex",
  "__complex__",
  "__imag",
  "__imag__",
  "__real",
  "__real__",
  "__signed__",
  "__unsigned__",
  "long",
  "short",
  "signed",
  "unsigned"
};

// MUST BE SORTED CASE-SENSITIVE
constexpr std::array builtin_type_suffix = {
  // Why, GCC, why?
  "_Complex",
  "__complex__",
};

// MUST BE SORTED CASE-SENSITIVE
constexpr std::array qualifiers = {
  "_Noreturn",
  "_Thread_local",
  "__const",
  "__extension__",
  "__inline",
  "__inline__",
  "__restrict",
  "__restrict__",
  "__stdcall",
  "__thread",
  "__volatile",
  "__volatile__",
  "auto",
  "const",
  "consteval",
  "constexpr",
  "constinit",
  "explicit",
  "extern",
  "inline",
  "mutable",
  "register",
  "restrict",
  "static",
  "thread_local",
  "virtual",
  "volatile",
};

// MUST BE SORTED CASE-SENSITIVE
constexpr std::array binary_operators = {
  "!=",
  "%",
  "%=",
  "&",
  "&&",
  "&=",
  "*",
  "*=",
  "+",
  "+=",
  "-",
  "-=",
  "->",
  "->*",
  ".",
  ".*",
  "/",
  "/=",
  "::",
  "<",
  "<<",
  "<<=",
  "<=",
  "<=>",
  "=",
  "==",
  ">",
  ">=",
  ">>",
  ">>=",
  "^",
  "^=",
  "|",
  "|=",
  "||",
};


constexpr int prefix_precedence(const char* op) {
  if (strcmp(op, "++")  == 0) return 3;
  if (strcmp(op, "--")  == 0) return 3;
  if (strcmp(op, "+")   == 0) return 3;
  if (strcmp(op, "-")   == 0) return 3;
  if (strcmp(op, "!")   == 0) return 3;
  if (strcmp(op, "~")   == 0) return 3;
  if (strcmp(op, "*")   == 0) return 3;
  if (strcmp(op, "&")   == 0) return 3;

  // 2 type(a) type{a}
  // 3 (type)a sizeof a sizeof(a) co_await a
  // 3 new a new a[] delete a delete a[]
  // 16 throw a co_yield a
  return 0;
}

constexpr int prefix_assoc(const char* op) {
  if (strcmp(op, "++")  == 0) return -1;
  if (strcmp(op, "--")  == 0) return -1;
  if (strcmp(op, "+")   == 0) return -1;
  if (strcmp(op, "-")   == 0) return -1;
  if (strcmp(op, "!")   == 0) return -1;
  if (strcmp(op, "~")   == 0) return -1;
  if (strcmp(op, "*")   == 0) return -1;
  if (strcmp(op, "&")   == 0) return -1;

  // 2 type(a) type{a}
  // 3 (type)a sizeof a sizeof(a) co_await a
  // 3 new a new a[] delete a delete a[]
  // 16 throw a co_yield a
  return 0;
}

constexpr int binary_precedence(const char* op) {
  if (strcmp(op, "::")  == 0) return 1;
  if (strcmp(op, ".")   == 0) return 2;
  if (strcmp(op, "->")  == 0) return 2;
  if (strcmp(op, ".*")  == 0) return 4;
  if (strcmp(op, "->*") == 0) return 4;
  if (strcmp(op, "*")   == 0) return 5;
  if (strcmp(op, "/")   == 0) return 5;
  if (strcmp(op, "%")   == 0) return 5;
  if (strcmp(op, "+")   == 0) return 6;
  if (strcmp(op, "-")   == 0) return 6;
  if (strcmp(op, "<<")  == 0) return 7;
  if (strcmp(op, ">>")  == 0) return 7;
  if (strcmp(op, "<=>") == 0) return 8;
  if (strcmp(op, "<")   == 0) return 9;
  if (strcmp(op, "<=")  == 0) return 9;
  if (strcmp(op, ">")   == 0) return 9;
  if (strcmp(op, ">=")  == 0) return 9;
  if (strcmp(op, "==")  == 0) return 10;
  if (strcmp(op, "!=")  == 0) return 10;
  if (strcmp(op, "&")   == 0) return 11;
  if (strcmp(op, "^")   == 0) return 12;
  if (strcmp(op, "|")   == 0) return 13;
  if (strcmp(op, "&&")  == 0) return 14;
  if (strcmp(op, "||")  == 0) return 15;
  if (strcmp(op, "?")   == 0) return 16;
  if (strcmp(op, ":")   == 0) return 16;
  if (strcmp(op, "=")   == 0) return 16;
  if (strcmp(op, "+=")  == 0) return 16;
  if (strcmp(op, "-=")  == 0) return 16;
  if (strcmp(op, "*=")  == 0) return 16;
  if (strcmp(op, "/=")  == 0) return 16;
  if (strcmp(op, "%=")  == 0) return 16;
  if (strcmp(op, "<<=") == 0) return 16;
  if (strcmp(op, ">>=") == 0) return 16;
  if (strcmp(op, "&=")  == 0) return 16;
  if (strcmp(op, "^=")  == 0) return 16;
  if (strcmp(op, "|=")  == 0) return 16;
  if (strcmp(op, ",")   == 0) return 17;
  return 0;
}

constexpr int binary_assoc(const char* op) {
  if (strcmp(op, "::")  == 0) return 1;
  if (strcmp(op, ".")   == 0) return 1;
  if (strcmp(op, "->")  == 0) return 1;
  if (strcmp(op, ".*")  == 0) return 1;
  if (strcmp(op, "->*") == 0) return 1;
  if (strcmp(op, "*")   == 0) return 1;
  if (strcmp(op, "/")   == 0) return 1;
  if (strcmp(op, "%")   == 0) return 1;
  if (strcmp(op, "+")   == 0) return 1;
  if (strcmp(op, "-")   == 0) return 1;
  if (strcmp(op, "<<")  == 0) return 1;
  if (strcmp(op, ">>")  == 0) return 1;
  if (strcmp(op, "<=>") == 0) return 1;
  if (strcmp(op, "<")   == 0) return 1;
  if (strcmp(op, "<=")  == 0) return 1;
  if (strcmp(op, ">")   == 0) return 1;
  if (strcmp(op, ">=")  == 0) return 1;
  if (strcmp(op, "==")  == 0) return 1;
  if (strcmp(op, "!=")  == 0) return 1;
  if (strcmp(op, "&")   == 0) return 1;
  if (strcmp(op, "^")   == 0) return 1;
  if (strcmp(op, "|")   == 0) return 1;
  if (strcmp(op, "&&")  == 0) return 1;
  if (strcmp(op, "||")  == 0) return 1;
  if (strcmp(op, "?")   == 0) return -1;
  if (strcmp(op, ":")   == 0) return -1;
  if (strcmp(op, "=")   == 0) return -1;
  if (strcmp(op, "+=")  == 0) return -1;
  if (strcmp(op, "-=")  == 0) return -1;
  if (strcmp(op, "*=")  == 0) return -1;
  if (strcmp(op, "/=")  == 0) return -1;
  if (strcmp(op, "%=")  == 0) return -1;
  if (strcmp(op, "<<=") == 0) return -1;
  if (strcmp(op, ">>=") == 0) return -1;
  if (strcmp(op, "&=")  == 0) return -1;
  if (strcmp(op, "^=")  == 0) return -1;
  if (strcmp(op, "|=")  == 0) return -1;
  if (strcmp(op, ",")   == 0) return 1;
  return 0;
}

constexpr int suffix_precedence(const char* op) {
  if (strcmp(op, "++")  == 0) return 2;
  if (strcmp(op, "--")  == 0) return 2;
  //if (strcmp(op, "(")   == 0) return 2;
  //if (strcmp(op, ")")   == 0) return 2;
  //if (strcmp(op, "[")   == 0) return 2;
  //if (strcmp(op, "]")   == 0) return 2;
  return 0;
}

constexpr int suffix_assoc(const char* op) {
  if (strcmp(op, "++")  == 0) return 1;
  if (strcmp(op, "--")  == 0) return 1;
  //if (strcmp(op, "(")   == 0) return 1;
  //if (strcmp(op, ")")   == 0) return 1;
  //if (strcmp(op, "[")   == 0) return 1;
  //if (strcmp(op, "]")   == 0) return 1;
  return 0;
}
