#pragma once
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
    for (auto s : old_slabs) delete [] s;
    old_slabs.clear();
    slab_cursor = 0;
    current_size = 0;
  }

  void* bump(size_t size) {
    if (slab_cursor + size > slab_size) {
      old_slabs.push_back(top_slab);
      top_slab = new uint8_t[slab_size];
      slab_cursor = 0;
    }

    auto result = top_slab + slab_cursor;
    slab_cursor += size;

    current_size += size;
    if (current_size > max_size) max_size = current_size;

    return result;
  }

  static constexpr size_t slab_size = 16*1024*1024;
  std::vector<uint8_t*> old_slabs;
  uint8_t* top_slab;
  size_t   slab_cursor;
  size_t   current_size;
  size_t   max_size;
};

//------------------------------------------------------------------------------
