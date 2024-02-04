#include <string>
#include <vector>
#include <stdio.h>
#include <algorithm>

#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

bool parse_json(const char* text, int size);

//------------------------------------------------------------------------------

void benchmark(const char* path) {
  int len = strlen(path);

  printf("Benchmarking %s", path);
  fflush(stdout);

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
    if (rep % 10 == 0) { printf("."); fflush(stdout); }
    auto time_a = utils::timestamp_ms();
    auto result = parse_json(buf.data(), buf.size());
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

  for (int i = 0; i < 22 - len; i++) printf(" ");

  printf("Rate %f MB/sec\n", (buf.size() / 1e6) / (parse_time / 1e3));
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {

  benchmark("canada.json");
  benchmark("rapidjson_sample.json");
  benchmark("twitter.json");
  benchmark("citm_catalog.json");

  return 0;
}

//------------------------------------------------------------------------------
