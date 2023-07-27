#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

int main(int argc, char** argv) {
  if (argc < 2) exit(-1);

  std::string input = utils::read(argv[1]);
  TextSpan text = utils::to_span(input);

  using pattern = Seq< Lit<"Hello">, Atom<' '>, Lit<"World"> >;

  TextContext ctx;
  TextSpan tail = pattern::match(ctx, text);
  utils::print_summary(text, tail, 50);

  return tail.is_valid() ? 0 : -1;
}
