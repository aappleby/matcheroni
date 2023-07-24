#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"
#include "matcheroni/dump.hpp"

using namespace matcheroni;

//------------------------------------------------------------------------------

struct JsonParser {
  static TextSpan match(TextContext& ctx, TextSpan body) {
    using sign      = Atom<'+', '-'>;
    using digit     = Range<'0', '9'>;
    using onenine   = Range<'1', '9'>;
    using digits    = Some<digit>;
    using integer   = Seq<Opt<Atom<'-'>>, Oneof<Seq<onenine, digits>, digit>>;
    using fraction  = Seq<Atom<'.'>, digits>;
    using exponent  = Seq<Atom<'e', 'E'>, Opt<sign>, digits>;
    using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;

    return number::match(ctx, body);
  }
};

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("0123456789012345678901234567890123456789");
  printf("0123456789012345678901234567890123456789");
  printf("\n");

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
  print_summary(text, tail, 50);

  return tail.is_valid() ? 0 : -1;
}
