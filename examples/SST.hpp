// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#include <stdio.h>

//------------------------------------------------------------------------------
// Sorted string table matcher thing.

template<const auto& table>
struct SST;

template<typename T, auto N, const std::array<T, N>& table>
struct SST<table> {

  constexpr static size_t top_bit(size_t x) {
    for (size_t b = 31; b >= 0; b--) {
      if (x & (1 << b)) return (1 << b);
    }
    return 0;
  }

  constexpr static bool contains(const char* text) {
    for (auto table_entry : table) {
      if (__builtin_strcmp(table_entry, text) == 0) return true;
    }
    return false;
  }

  inline static int strcmp_span(const char* a, const char* b, const char* lit) {
    while (1) {
      auto ca = a == b ? 0 : *a;
      auto cb = *lit;
      if (ca != cb || ca == 0) return ca - cb;
      a++;
      lit++;
    }
  }

  static const char* match(const char* a, const char* b) {
    if (!a || *a == 0) return nullptr;
    size_t bit = top_bit(N);
    size_t index = 0;

    // I'm not actually sure if 8 is the best tradeoff but it seems OK
    if (N > 8) {
      // Binary search for large tables
      while(1) {
        size_t new_index = index | bit;
        if (new_index < N) {
          auto lit = table[new_index];
          auto c = strcmp_span(a, b, lit);
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
        if (strcmp_span(a, b, lit) == 0) return lit;
      }
    }

    return nullptr;
  }
};

//------------------------------------------------------------------------------
