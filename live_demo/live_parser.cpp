#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"

using namespace matcheroni;
using namespace parseroni;

//------------------------------------------------------------------------------

using sign      = Atom<'+', '-'>;
using digit     = Range<'0', '9'>;
using onenine   = Range<'1', '9'>;
using digits    = Some<digit>;
using integer   = Seq<Opt<Atom<'-'>>, Oneof<Seq<onenine, digits>, digit>>;
using fraction  = Seq<Atom<'.'>, digits>;
using exponent  = Seq<Atom<'e', 'E'>, Opt<sign>, digits>;
using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;

//------------------------------------------------------------------------------

using ws     = Some<Atom<' ', '\n', '\r', '\t'>>;
using hex       = Range<'0','9','a','f','A','F'>;
using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
using character = Oneof<
  Seq<Not<Atom<'"'>>, Not<Atom<'\\'>>, Range<0x0020, 0x10FFFF>>,
  Seq<Atom<'\\'>, escape>
>;
using string = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

//------------------------------------------------------------------------------

using keyword = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;

template <typename P>
using list = Seq<P, Any<Seq<Opt<ws>, Atom<','>, Opt<ws>, P>>>;

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

using array = Seq<Atom<'['>, Opt<ws>, Opt<list<value>>, Opt<ws>, Atom<']'>>;

using pair =
Seq<
  Capture<"key", string, TextParseNode>,
  Opt<ws>,
  Atom<':'>,
  Opt<ws>,
  Capture<"value", value, TextParseNode>
>;

using object = Seq<
  Atom<'{'>,
  Opt<ws>,
  Opt<list<Capture<"member", pair, TextParseNode>>>,
  Opt<ws>,
  Atom<'}'>
>;

using json = Seq<Opt<ws>, value, Opt<ws>>;

void parse_json(const char* text, int size, bool verbose) {
}
