// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vector>

//------------------------------------------------------------------------------

struct SlabAlloc {

  SlabAlloc() {
    top_slab = new uint8_t[slab_size];
    slab_cursor = 0;
    current_size = 0;
    max_size = 0;
  }

  void reset() {
    for (auto s : old_slabs) new_slabs.push_back(s);
    old_slabs.clear();
    slab_cursor = 0;
    current_size = 0;
  }

  void* bump(size_t size) {
    if (slab_cursor + size > slab_size) {
      old_slabs.push_back(top_slab);
      if (new_slabs.empty()) {
        top_slab = new uint8_t[slab_size];
      }
      else {
        top_slab = new_slabs.back();
        new_slabs.pop_back();
      }
      slab_cursor = 0;
    }

    auto result = top_slab + slab_cursor;
    slab_cursor += size;

    current_size += size;
    if (current_size > max_size) max_size = current_size;

    return result;
  }

  // slab size is 1 hugepage. seems to work ok.
  static constexpr size_t slab_size = 2*1024*1024;
  std::vector<uint8_t*> old_slabs;
  std::vector<uint8_t*> new_slabs;
  uint8_t* top_slab;
  size_t   slab_cursor;
  size_t   current_size;
  size_t   max_size;
};

//------------------------------------------------------------------------------
