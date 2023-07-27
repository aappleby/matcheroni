#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

struct JsonMatcher {
  // Matches any JSON number
  using sign      = Atom<'+', '-'>;
  using digit     = Range<'0', '9'>;
  using onenine   = Range<'1', '9'>;
  using digits    = Some<digit>;
  using integer   = Seq<Opt<sign>, Oneof<Seq<onenine, digits>, digit>>;
  using fraction  = Seq<Atom<'.'>, digits>;
  using exponent  = Seq<Atom<'e', 'E'>, Opt<sign>, digits>;
  using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;

  // Matches a JSON string that can contain valid escape characters
  using space     = Some<Atom<' ', '\n', '\r', '\t'>>;
  using hex       = Range<'0','9','a','f','A','F'>;
  using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
  using character = Oneof<
    Seq<Not<Atom<'"'>>, Not<Atom<'\\'>>, Range<0x0020, 0x10FFFF>>,
    Seq<Atom<'\\'>, escape>
  >;
  using string = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

  // Matches the three reserved JSON keywords
  using keyword = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;

  // Matches the above items separated by whitespace
  static TextSpan match(TextContext& ctx, TextSpan body) {
    using item = Oneof<number, string, keyword>;
    using pattern = Seq<item, Any<Seq<space, item>>>;
    return pattern::match(ctx, body);
  }
};

int main(int argc, char** argv) {
  if (argc < 2) exit(-1);

  std::string input = utils::read(argv[1]);
  TextSpan text = utils::to_span(input);

  TextContext ctx;
  auto tail = JsonMatcher::match(ctx, text);
  utils::print_summary(text, tail, 50);

  return tail.is_valid() ? 0 : -1;
}
