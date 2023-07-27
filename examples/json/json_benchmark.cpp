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

#if 0
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
const int reps = 20;
#endif

TextSpan parse_json(TextNodeContext& ctx, TextSpan body);

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Matcheroni JSON parsing benchmark\n");

  const char* paths[] = {
    //"data/json_demo.json",
    //"data/invalid.json",
    //"data/sample.json",


    // 4609770.000000
    "data/canada.json",
    "data/citm_catalog.json",
    "data/twitter.json",
  };

  double byte_accum = 0;
  double time_accum = 0;
  double line_accum = 0;

  TextNodeContext ctx;

  double min_rep_accum = 0;

  for (auto path : paths) {
    if (verbose) {
      printf("\n\n----------------------------------------\n");
      printf("Parsing %s\n", path);
    }

    char* buf = nullptr;
    size_t size = 0;
    utils::read(path, buf, size);
    if (size == 0) {
      printf("Could not load %s\n", path);
      continue;
    }

    byte_accum += size;
    for (size_t i = 0; i < size; i++) if (buf[i] == '\n') line_accum++;

    TextSpan text = {buf, buf + size};
    TextSpan parse_end = text;

    //----------------------------------------

    double min_rep = 1e100;
    double rep_time_accum = 0;

    for (int rep = 0; rep < (warmup + reps); rep++) {
      ctx.reset();

      double time_a = utils::timestamp_ms();
      parse_end = parse_json(ctx, text);
      double time_b = utils::timestamp_ms();

      double time = time_b - time_a;

      if (rep >= warmup) rep_time_accum += time;
      if (time < min_rep) min_rep = time;
    }

    time_accum += rep_time_accum;
    min_rep_accum += min_rep;

    //----------------------------------------

    if (parse_end.begin < text.end) {
      printf("Parse failed!\n");
      printf("Failure near `");
      //print_flat(TextSpan(context->highwater, text.b), 20);
      printf("`\n");
      continue;
    }

    if (verbose) {
      printf("Parsed %d reps in %f msec\n", reps, rep_time_accum);
      printf("\n");
    }

    if (dump_tree) {
      printf("Parse tree:\n");
      utils::print_context(text, ctx, 40);
    }

    if (verbose) {
      printf("Slab current      %d\n",  LinearAlloc::inst().current_size);
      printf("Slab max          %d\n",  LinearAlloc::inst().max_size);
      printf("Tree nodes        %ld\n", ctx.node_count());
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
  printf("Avg rep time %f\n", time_accum / reps);
  printf("Min rep time %f\n", min_rep_accum);
  printf("\n");

  return 0;
}

//------------------------------------------------------------------------------
