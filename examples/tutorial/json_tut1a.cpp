#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

struct JsonMatcher {
  // Matches any JSON number
  using sign      = Atoms<'+', '-'>;
  using digit     = Range<'0', '9'>;
  using onenine   = Range<'1', '9'>;
  using digits    = Some<digit>;
  using integer   = Seq<Opt<Atom<'-'>>, Oneof<Seq<onenine, digits>, digit>>;
  using fraction  = Seq<Atom<'.'>, digits>;
  using exponent  = Seq<Atoms<'e', 'E'>, Opt<sign>, digits>;
  using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;

  static TextSpan match(TextMatchContext& ctx, TextSpan body) {
    return number::match(ctx, body);
  }
};

int main(int argc, char** argv) {
  const char* filename = argc < 2 ? "examples/tutorial/json_tut1a.input" : argv[1];

  std::string input = utils::read(filename);
  TextSpan text = utils::to_span(input);

  TextMatchContext ctx;
  auto tail = JsonMatcher::match(ctx, text);
  utils::print_summary(text, tail, 50);

  return tail.is_valid() ? 0 : -1;
}
