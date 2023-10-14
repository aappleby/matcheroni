#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

#include <string>
#include <vector>
#include <stdio.h>
#include <algorithm>

using namespace matcheroni;
using namespace parseroni;

// Benchmark config
constexpr bool verbose   = false;
constexpr bool dump_tree = false;
const int warmup = 0;
const int reps = 100;

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: json_demo <filename>\n");
    return 1;
  }

  const char* path = "twitter.json";

  printf("----------------------------------------\n");
  printf("Parsing %s\n", path);

  std::string buf;
  utils::read(path, buf);
  if (buf.size() == 0) {
    printf("Could not load %s\n", path);
    exit(-1);
  }

  TextSpan text = utils::to_span(buf);

  //----------------------------------------

  TextParseContext ctx2;
  TextSpan parse_end = text;
  std::vector<double> parse_times;
  parse_times.reserve(reps);
  for (int rep = 0; rep < reps; rep++) {


    auto time_a = utils::timestamp_ms();
    ctx2.reset();
    parse_end = parse_json(ctx2, text);
    auto time_b = utils::timestamp_ms();

    if (parse_end.begin < text.end) {
      printf("Parse failed!\n");
      printf("Failure near `");
      printf("%50.50s\n", ctx2._highwater);
      printf("`\n");
      exit(-1);
    }

    if (rep >= warmup) {
      parse_times.push_back(time_b - time_a);
    }
  }

  //----------------------------------------

  std::sort(parse_times.begin(), parse_times.end());
  auto parse_time = parse_times[parse_times.size()/2];

  if (dump_tree) {
    printf("Parse tree:\n");
    utils::print_summary(ctx2, text, parse_end, 40);
  }

  printf("\n");
  printf("Byte total %f\n", buf.size());
  printf("Parse time %f\n", parse_time);
  printf("Parse byte rate  %f megabytes per second\n", (buf.size() / 1e6) / (parse_time / 1e3));

  return 0;
}
