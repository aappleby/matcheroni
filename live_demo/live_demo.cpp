#include <string>
#include <vector>
#include <stdio.h>
#include <algorithm>

#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

bool parse_json(const std::string& text, bool verbose);

//------------------------------------------------------------------------------

void benchmark(const char* path) {
  printf("Benchmarking %s: ", path);

  std::string buf;
  utils::read(path, buf);
  if (buf.size() == 0) {
    printf("Could not load %s\n", path);
    exit(-1);
  }

  //----------------------------------------

  const int warmup = 20;
  const int reps = 100;
  std::vector<double> parse_times;
  parse_times.reserve(reps);
  for (int rep = 0; rep < warmup + reps; rep++) {
    auto time_a = utils::timestamp_ms();
    auto result = parse_json(buf, false);
    auto time_b = utils::timestamp_ms();
    if (!result) {
      printf("Parse FAIL!\n");
      exit(-1);
    }

    if (rep >= warmup) parse_times.push_back(time_b - time_a);
  }

  //----------------------------------------

  std::sort(parse_times.begin(), parse_times.end());
  auto parse_time = parse_times[parse_times.size()/2];

  printf("Rate %f MB/sec\n", (buf.size() / 1e6) / (parse_time / 1e3));
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {

  const char* path = "data.json";

  printf("Parsing %s:\n", path);

  std::string buf;
  utils::read(path, buf);
  if (buf.size() == 0) {
    printf("Could not load %s\n", path);
    exit(-1);
  }

  printf("%s\n", buf.data());

  parse_json(buf, true);
  printf("\n");

  benchmark("canada.json");
  benchmark("rapidjson_sample.json");
  benchmark("twitter.json");
  benchmark("citm_catalog.json");

  return 0;
}

//------------------------------------------------------------------------------
