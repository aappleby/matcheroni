//------------------------------------------------------------------------------
// This file is a full working example of using Matcheroni to build a JSON
// parser.

// Example usage:
// bin/json_parser test_file.json

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "json.hpp"
#include "matcheroni/Utilities.hpp"

#include <stdio.h>
#include <algorithm>

using namespace matcheroni;

#if 0
// Debug config
constexpr bool verbose   = true;
constexpr bool dump_tree = true;
const int reps = 1;
#else
// Benchmark config
constexpr bool verbose   = false;
constexpr bool dump_tree = false;
const int reps = 100;
#endif

#define MATCH
#define PARSE

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Matcheroni JSON matching/parsing benchmark\n");

  const char* paths[] = {
    "data/canada.json",
    "data/citm_catalog.json",
    "data/twitter.json",
    "data/rapidjson_sample.json",
  };

  double all_byte_accum = 0;
  double all_line_accum = 0;
  double all_match_time = 0;
  double all_parse_time = 0;

  TextMatchContext ctx1;
  JsonContext ctx2;

  for (auto path : paths) {
    double byte_accum = 0;
    double line_accum = 0;
    double match_time = 0;
    double parse_time = 0;

    printf("----------------------------------------\n");
    printf("Parsing %s\n", path);

    std::string buf;
    utils::read(path, buf);
    if (buf.size() == 0) {
      printf("Could not load %s\n", path);
      continue;
    }

    byte_accum += buf.size();
    for (size_t i = 0; i < buf.size(); i++) if (buf[i] == '\n') line_accum++;

    TextSpan text = utils::to_span(buf);

    //----------------------------------------

    TextSpan match_end = text;
    double best_match = 1.0e100;
    std::vector<double> match_times;
    match_times.reserve(reps);
    for (int rep = 0; rep < reps; rep++) {
      double time = -utils::timestamp_ms();
#ifdef MATCH
      match_end = match_json(ctx1, text);
#endif
      time += utils::timestamp_ms();
      match_times.push_back(time);
    }
    std::sort(match_times.begin(), match_times.end());
    match_time += match_times[reps/2];

#ifdef MATCH
    if (match_end.begin < text.end) {
      printf("Match failed!\n");
      printf("Failure near `");
      printf("%50.50s", match_end.begin);
      printf("`\n");
      exit(-1);
    }
#endif

    //----------------------------------------

    TextSpan parse_end = text;
    std::vector<double> parse_times;
    parse_times.reserve(reps);
    for (int rep = 0; rep < reps; rep++) {
      double time = -utils::timestamp_ms();
      ctx2.reset();
#ifdef PARSE
      parse_end = parse_json(ctx2, text);
#endif
      time += utils::timestamp_ms();
      parse_times.push_back(time);
    }
    std::sort(parse_times.begin(), parse_times.end());
    parse_time += parse_times[reps/2];

#ifdef PARSE
    if (parse_end.begin < text.end) {
      printf("Parse failed!\n");
      printf("Failure near `");
      printf("%50.50s\n", ctx2._highwater);
      printf("`\n");
      exit(-1);
    }
#endif

    //----------------------------------------

    if (dump_tree) {
      printf("Parse tree:\n");
      utils::print_summary(ctx2, text, parse_end, 40);
    }

    if (verbose) {
      //printf("Slab current      %d\n",  LifoAlloc::inst().current_size);
      //printf("Slab max          %d\n",  LifoAlloc::inst().max_size);
      //printf("Sizeof(node) == %ld\n", sizeof(JsonNode));
    }

    printf("\n");
    printf("Tree nodes %ld\n", ctx2.node_count());
    printf("Byte total %f\n", byte_accum);
    printf("Line total %f\n", line_accum);
    printf("Match time %f\n", match_time);
    printf("Parse time %f\n", parse_time);
    printf("Match byte rate  %f megabytes per second\n", (byte_accum / 1e6) / (match_time / 1e3));
    printf("Match line rate  %f megalines per second\n", (line_accum / 1e6) / (match_time / 1e3));
    printf("Parse byte rate  %f megabytes per second\n", (byte_accum / 1e6) / (parse_time / 1e3));
    printf("Parse line rate  %f megalines per second\n", (line_accum / 1e6) / (parse_time / 1e3));

    all_byte_accum += byte_accum;
    all_line_accum += line_accum;
    all_match_time += match_time;
    all_parse_time += parse_time;
  }

  printf("----------------------------------------\n");
  printf("Results averaged over all test files:\n");
  printf("\n");
  printf("Byte total %f\n", all_byte_accum);
  printf("Line total %f\n", all_line_accum);
  printf("Match time %f\n", all_match_time);
  printf("Parse time %f\n", all_parse_time);
  printf("Match byte rate  %f megabytes per second\n", (all_byte_accum / 1e6) / (all_match_time / 1e3));
  printf("Match line rate  %f megalines per second\n", (all_line_accum / 1e6) / (all_match_time / 1e3));
  printf("Parse byte rate  %f megabytes per second\n", (all_byte_accum / 1e6) / (all_parse_time / 1e3));
  printf("Parse line rate  %f megalines per second\n", (all_line_accum / 1e6) / (all_parse_time / 1e3));
  printf("\n");

  return 0;
}

//------------------------------------------------------------------------------
