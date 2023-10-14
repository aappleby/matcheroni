//------------------------------------------------------------------------------
// This file uses JSON conformance tests from
// https://github.com/nst/JSONTestSuite to verify that the JSON parser conforms
// with the http://JSON.org spec (except for the two that blow up the stack of
// a recursive parser :D )

// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "json.hpp"
#include "matcheroni/Utilities.hpp"

#include <sys/resource.h>
#include <stdio.h>
#include <filesystem>
#include <vector>
#include <string>

using namespace matcheroni;

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  auto base_path = "conformance";

  std::vector<std::string> tests_good;
  std::vector<std::string> tests_bad;
  std::vector<std::string> tests_other;

  printf("Scanning source files in %s\n", base_path);
  using rdit = std::filesystem::recursive_directory_iterator;
  for (const auto& f : rdit(base_path)) {
    if (!f.is_regular_file()) continue;
    auto path = f.path().native();
    auto name = f.path().filename().native();

    if (name.starts_with("i_") && name.ends_with(".json")) {
      tests_other.push_back(path);
    }

    if (name.starts_with("y_") && name.ends_with(".json")) {
      tests_good.push_back(path);
    }

    if (name.starts_with("n_") && name.ends_with(".json")) {
      tests_bad.push_back(path);
    }
  }

  double time;

  int y_pass = 0;
  int y_fail = 0;

  printf("tests_good... ");
  time = 0;
  for (auto path : tests_good) {
    std::string raw_text;
    utils::read(path.c_str(), raw_text);

    TextMatchContext ctx2;
    JsonContext ctx;
    TextSpan text = utils::to_span(raw_text);
    time -= utils::timestamp_ms();
    TextSpan tail = parse_json(ctx, text);
    time += utils::timestamp_ms();

    if (tail.is_valid() && tail.begin == text.end) {
      y_pass++;
    }
    else {
      y_fail++;
      printf("\n");
      printf("FAIL %s\n", path.c_str());
    }
  }
  printf("%f msec\n", time);

  int n_pass = 0;
  int n_fail = 0;
  int skipped = 0;

  printf("tests_bad... ");
  time = 0;
  for (auto path : tests_bad) {
    // We are a recursive descent parser, these blow our call stack
    if (path == "conformance/n_structure_open_array_object.json") {
      skipped++;
      continue;
    }
    if (path == "conformance/n_structure_100000_opening_arrays.json") {
      skipped++;
      continue;
    }

    std::string raw_text;
    utils::read(path.c_str(), raw_text);

    JsonContext ctx;
    TextSpan text = utils::to_span(raw_text);
    time -= utils::timestamp_ms();
    TextSpan tail = parse_json(ctx, text);
    time += utils::timestamp_ms();

    if (tail.is_valid() && tail.begin == text.end) {
      n_fail++;
      printf("FAIL %s\n", path.c_str());
    }
    else {
      n_pass++;
    }
  }
  printf("%f msec\n", time);

  int i_pass = 0;
  int i_fail = 0;

  printf("tests_other... ");
  time = 0;
  for (auto path : tests_other) {
    std::string raw_text;
    utils::read(path.c_str(), raw_text);

    JsonContext ctx;
    TextSpan text = utils::to_span(raw_text);
    time -=  utils::timestamp_ms();
    TextSpan tail = parse_json(ctx, text);
    time +=  utils::timestamp_ms();

    if (tail.is_valid() && tail.begin == text.end) {
      i_pass++;
    }
    else {
      i_fail++;
    }
  }
  printf("%f msec\n", time);

  printf("Known good pass %d\n", y_pass);
  printf("Known good fail %d\n", y_fail);
  printf("Known bad pass  %d\n", n_pass);
  printf("Known bad fail  %d\n", n_fail);
  printf("Other pass      %d\n", i_pass);
  printf("Other fail      %d\n", i_fail);
  printf("Skipped         %d\n", skipped);

  return (y_fail || n_fail) ? -1 : 0;
}

//------------------------------------------------------------------------------
