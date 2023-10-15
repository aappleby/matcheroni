#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"
#include <string>

using namespace matcheroni;
using namespace parseroni;

//------------------------------------------------------------------------------
// Numbers

using sign     = Atom<'+', '-'>;
using digit    = Range<'0', '9'>;
using onenine  = Range<'1', '9'>;
using digits   = Some<digit>;
using integer  = Seq<Opt<Atom<'-'>>, Oneof<Seq<onenine, digits>, digit>>;
using fraction = Seq<Atom<'.'>, digits>;
using exponent = Seq<Atom<'e', 'E'>, Opt<sign>, digits>;
using number   = Seq<integer, Opt<fraction>, Opt<exponent>>;

//------------------------------------------------------------------------------
// Strings

using ws = Any<Atom<' ', '\n', '\r', '\t'>>;
using hex = Range<'0','9','a','f','A','F'>;

using escape =
Seq<
  Atom<'\\'>,
  Oneof<
    Charset<"\"\\/bfnrt">,
    Seq<Atom<'u'>, Rep<4, hex>>
  >
>;

using character = Oneof<
  Seq<
    Not<Atom<'"'>>,
    Not<Atom<'\\'>>,
    Range<0x0020, 0x10FFFF>
  >,
  escape
>;

using string = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

//------------------------------------------------------------------------------
// Arrays

using JsonContext = matcheroni::TextMatchContext;
TextSpan match_value(JsonContext& ctx, TextSpan body);

template <typename pattern>
using list =
Seq<
  pattern,
  Any<Seq<
    ws,
    Atom<','>,
    ws,
    pattern
  >>
>;

using value  = Ref<match_value>;

using array =
Seq<
  Atom<'['>,
  ws,
  Opt<list<value>>,
  ws,
  Atom<']'>
>;

//------------------------------------------------------------------------------
// Objects

using key = string;

using member =
Seq<
  key,
  ws,
  Atom<':'>,
  ws,
  value
>;

using object =
Seq<
  Atom<'{'>,
  ws,
  Opt<list<member>>,
  ws,
  Atom<'}'>
>;

using json = Seq<ws, value, ws>;

TextSpan match_value(JsonContext& ctx, TextSpan body) {
  using value =
  Oneof<
    number,
    string,
    array,
    object,
    Lit<"true">,
    Lit<"false">,
    Lit<"null">
  >;
  return value::match(ctx, body);
}

//------------------------------------------------------------------------------
// Parser

bool parse_json(const std::string& text, bool verbose) {
  TextSpan body(text.data(), text.data() + text.size());

  static JsonContext ctx;
  ctx.reset();
  auto result = json::match(ctx, body);

  return result.is_valid();
}

//------------------------------------------------------------------------------
