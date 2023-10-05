#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

int main(int argc, char** argv) {
  const char* filename = argc < 2 ? "tutorial/json_tut0a.input" : argv[1];

  std::string input = utils::read(filename);
  TextSpan text = utils::to_span(input);

  using pattern = Seq< Lit<"Hello">, Atom<' '>, Lit<"World"> >;

  TextMatchContext ctx;
  TextSpan tail = pattern::match(ctx, text);
  utils::print_summary(text, tail, 50);

  return tail.is_valid() ? 0 : -1;
}
