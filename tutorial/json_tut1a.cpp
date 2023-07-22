#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"
#include "matcheroni/dump.hpp"

using namespace matcheroni;

//------------------------------------------------------------------------------

struct JsonParser {
  // clang-format off
  using sign      = Atom<'+', '-'>;
  using digit     = Range<'0', '9'>;
  using onenine   = Range<'1', '9'>;
  using digits    = Some<digit>;
  using integer   = Seq<Opt<Atom<'-'>>, Oneof<Seq<onenine, digits>, digit>>;
  using fraction  = Seq<Atom<'.'>, digits>;
  using exponent  = Seq<Atom<'e', 'E'>, Opt<sign>, digits>;
  using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;
  // clang-format on

  static TextSpan match(TextContext& ctx, TextSpan body) {
    return number::match(ctx, body);
  }
};

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("json_tut1a <filename>\n");
    return 0;
  }

  printf("argv[0] = %s\n", argv[0]);
  printf("argv[1] = %s\n", argv[1]);
  printf("\n");

  TextContext ctx;
  auto input = read(argv[1]);
  auto text = to_span(input);
  auto tail = JsonParser::match(ctx, text);
  print_summary(text, tail, 40);

  return tail.is_valid() ? 0 : -1;
}
