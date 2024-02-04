#include <string>
#include <vector>
#include <stdio.h>
#include <algorithm>

#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

bool parse_json(const char* text, int size);

//------------------------------------------------------------------------------

int main(int argc, char** argv) {

  const char* path = "data.json";

  printf("Parsing %s: ", path);

  std::string buf;
  utils::read(path, buf);
  if (buf.size() == 0) {
    printf("Could not load %s\n", path);
    exit(-1);
  }

  printf("%s\n", buf.data());

  auto result = parse_json(buf.data(), buf.size());
  printf(result ? "Parse OK!\n" : "Parse fail!\n");

  return 0;
}

//------------------------------------------------------------------------------
