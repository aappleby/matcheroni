#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;
using namespace parseroni;

struct JsonParser {
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

  // Matches a comma-delimited list with embedded whitespace
  template <typename P>
  using list = Seq<P, Any<Seq<Opt<space>, Atom<','>, Opt<space>, P>>>;

  // Matches any valid JSON value
  static TextSpan match_value(TextParseContext& ctx, TextSpan body) {
    return Oneof<
      Capture<"number",  number,  TextParseNode>,
      Capture<"string",  string,  TextParseNode>,
      Capture<"array",   array,   TextParseNode>,
      Capture<"object",  object,  TextParseNode>,
      Capture<"keyword", keyword, TextParseNode>
    >::match(ctx, body);
  }
  using value = Ref<match_value>;

  // Matches bracket-delimited lists of JSON values
  using array =
  Seq<
    Atom<'['>,
    Opt<space>,
    Opt<list<value>>,
    Opt<space>,
    Atom<']'>
  >;

  // Matches a key:value pair where 'key' is a string and 'value' is a JSON value.
  using pair =
  Seq<
    Capture<"key", string, TextParseNode>,
    Opt<space>,
    Atom<':'>,
    Opt<space>,
    Capture<"value", value, TextParseNode>
  >;

  // Matches a curly-brace-delimited list of key:value pairs.
  using object =
  Seq<
    Atom<'{'>,
    Opt<space>,
    Opt<list<Capture<"member", pair, TextParseNode>>>,
    Opt<space>,
    Atom<'}'>
  >;

  // Matches any valid JSON document
  static TextSpan match(TextParseContext& ctx, TextSpan body) {
    return Seq<Opt<space>, value, Opt<space>>::match(ctx, body);
  }
};

int main(int argc, char** argv) {
  if (argc < 2) exit(-1);

  std::string input = utils::read(argv[1]);
  TextSpan text = utils::to_span(input);

  TextParseContext ctx;
  TextSpan tail = JsonParser::match(ctx, text);
  utils::print_summary(ctx, text, tail, 50);

  return tail.is_valid() ? 0 : -1;
}
