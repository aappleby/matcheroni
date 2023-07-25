#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

//using namespace matcheroni;

using matcheroni::Lit;
using matcheroni::TextContext;
using matcheroni::TextSpan;
using matcheroni::utils::read;
using matcheroni::utils::to_span;
using matcheroni::utils::print_summary;

int main(int argc, char** argv) {
  if (argc < 2) exit(-1);

  TextContext ctx;
  auto input = read(argv[1]);
  auto text = to_span(input);
  auto tail = Lit<"Hello">::match(ctx, text);
  print_summary(text, tail, 50);

  return tail.is_valid() ? 0 : -1;
}
