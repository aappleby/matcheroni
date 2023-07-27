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

#define MATCH
#define PARSE

TextSpan match_json(TextContext& ctx, TextSpan body);
TextSpan parse_json(TextNodeContext& ctx, TextSpan body);

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Matcheroni JSON matching/parsing benchmark\n");

  const char* paths[] = {
    // should be 4609770 bytes
    "data/canada.json",
    "data/citm_catalog.json",
    "data/twitter.json",
  };

  double byte_accum = 0;
  double match_accum = 0;
  double parse_accum = 0;
  double line_accum = 0;

  TextContext ctx1;
  TextNodeContext ctx2;

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

    //----------------------------------------

#ifdef MATCH
    TextSpan match_end = text;
    for (int rep = 0; rep < (warmup + reps); rep++) {
      if (rep >= warmup) match_accum -= utils::timestamp_ms();
      match_end = match_json(ctx1, text);
      if (rep >= warmup) match_accum += utils::timestamp_ms();
    }

    if (match_end.begin < text.end) {
      printf("Match failed!\n");
      printf("Failure near `");
      printf("%50.50s", match_end.begin);
      printf("`\n");
      continue;
    }
#endif

    //----------------------------------------

#ifdef PARSE
    TextSpan parse_end = text;
    for (int rep = 0; rep < (warmup + reps); rep++) {
      if (rep >= warmup) parse_accum -= utils::timestamp_ms();
      ctx2.reset();
      parse_end = parse_json(ctx2, text);
      if (rep >= warmup) parse_accum += utils::timestamp_ms();
    }

    if (parse_end.begin < text.end) {
      printf("Parse failed!\n");
      printf("Failure near `");
      printf("%50.50s", parse_end.begin);
      printf("`\n");
      continue;
    }
#endif

    //----------------------------------------

    if (dump_tree) {
      printf("Parse tree:\n");
      utils::print_context(text, ctx2, 40);
    }

    if (verbose) {
      printf("Slab current      %d\n",  LinearAlloc::inst().current_size);
      printf("Slab max          %d\n",  LinearAlloc::inst().max_size);
      printf("Tree nodes        %ld\n", ctx2.node_count());
    }

    delete [] buf;
  }

  match_accum /= reps;
  parse_accum /= reps;

  printf("\n");
  printf("----------------------------------------\n");
  printf("Byte total %f\n", byte_accum);
  printf("Line total %f\n", line_accum);
  printf("Match time %f\n", match_accum);
  printf("Parse time %f\n", parse_accum);
  printf("Match byte rate  %f megabytes per second\n", (byte_accum / 1e6) / (match_accum / 1e3));
  printf("Match line rate  %f megalines per second\n", (line_accum / 1e6) / (match_accum / 1e3));
  printf("Parse byte rate  %f megabytes per second\n", (byte_accum / 1e6) / (parse_accum / 1e3));
  printf("Parse line rate  %f megalines per second\n", (line_accum / 1e6) / (parse_accum / 1e3));
  printf("\n");

  return 0;
}

//------------------------------------------------------------------------------
