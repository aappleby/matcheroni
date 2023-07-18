//------------------------------------------------------------------------------
// This file is a full working example of using Matcheroni to build a JSON
// parser.

// Example usage:
// bin/json_parser test_file.json

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

#include <stdio.h>

using namespace matcheroni;

#if 1
// Debug config
constexpr bool verbose   = true;
constexpr bool dump_tree = true;
const int warmup = 0;
const int reps = 1;
#else
// Benchmark config
constexpr bool verbose   = false;
constexpr bool dump_tree = false;
const int warmup = 10;
const int reps = 10;
#endif

TextSpan parse_json(void* ctx, TextSpan s);

//------------------------------------------------------------------------------

int main(int argc, char** argv) {

  if (argc < 2) {
    printf("Usage: json_demo <filename>\n");
    return 1;
  }

  printf("argv[0] = %s\n", argv[0]);
  printf("argv[1] = %s\n", argv[1]);

  const char* paths[] = {
    argv[1]
  };

  double byte_accum = 0;
  double time_accum = 0;
  double line_accum = 0;

  TextContext* context = new TextContext();

  for (auto path : paths) {
    if (verbose) {
      printf("\n\n----------------------------------------\n");
      printf("Parsing %s\n", path);
    }

    char* buf = nullptr;
    size_t size = 0;
    read(path, buf, size);
    if (size == 0) {
      printf("Could not load %s\n", path);
      continue;
    }

    byte_accum += size;
    for (size_t i = 0; i < size; i++) if (buf[i] == '\n') line_accum++;

    TextSpan text = {buf, buf + size};
    TextSpan parse_end = text;

    //----------------------------------------

    double path_time_accum = 0;

    for (int rep = 0; rep < (warmup + reps); rep++) {
      context->reset();

      double time_a = timestamp_ms();
      parse_end = parse_json(context, text);
      double time_b = timestamp_ms();

      if (rep >= warmup) path_time_accum += time_b - time_a;
    }

    time_accum += path_time_accum;

    //----------------------------------------

    if (parse_end.a < text.b) {
      printf("Parse failed!\n");
      printf("Failure near `");
      //print_flat(TextSpan(context->highwater, text.b), 20);
      printf("`\n");
      continue;
    }

    if (verbose) {
      printf("Parsed %d reps in %f msec\n", reps, path_time_accum);
      printf("\n");
    }

    if (dump_tree) {
      printf("Parse tree:\n");
      for (auto n = context->top_head(); n; n = n->node_next()) {
        print_tree(n);
      }
    }

    if (verbose) {
      printf("Slab current      %d\n",  LinearAlloc::inst().current_size);
      printf("Slab max          %d\n",  LinearAlloc::inst().max_size);
      printf("Tree nodes        %ld\n", context->node_count());
      //printf("Constructor calls %ld\n", NodeBase<char>::constructor_calls);
      //printf("Destructor calls  %ld\n", NodeBase<char>::destructor_calls);
    }

    delete [] buf;
  }

  printf("\n");
  printf("----------------------------------------\n");
  printf("Byte accum %f\n", byte_accum);
  printf("Time accum %f\n", time_accum);
  printf("Line accum %f\n", line_accum);
  printf("Byte rate  %f\n", byte_accum / (time_accum / 1000.0));
  printf("Line rate  %f\n", line_accum / (time_accum / 1000.0));
  printf("Rep time   %f\n", time_accum / reps);
  printf("\n");

  delete context;

  return 0;
}

//------------------------------------------------------------------------------
