#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"
#include "matcheroni/dump.hpp"

using namespace matcheroni;

//------------------------------------------------------------------------------

struct JsonParser {
  static TextSpan match(TextContext& ctx, TextSpan body) {
    using sign      = Opt<Atom<'+', '-'>>;
    using digit     = Range<'0', '9'>;
    using onenine   = Range<'1', '9'>;
    using digits    = Some<digit>;
    using integer   = Seq<sign, Oneof<Seq<onenine, digits>, digit>>;
    using fraction  = Seq<Atom<'.'>, digits>;
    using exponent  = Seq<Atom<'e', 'E'>, Opt<sign>, digits>;
    using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;

    return number::match(ctx, body);
  }
};

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  if (argc < 2) exit(-1);

  TextContext ctx;
  auto input = utils::read(argv[1]);
  auto text = utils::to_span(input);
  auto tail = JsonParser::match(ctx, text);
  utils::print_summary(text, tail, 50);

  return tail.is_valid() ? 0 : -1;
}
