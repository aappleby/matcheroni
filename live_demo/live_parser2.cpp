#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"

#ifdef DEBUG
#include "matcheroni/Utilities.hpp"
#endif

using namespace matcheroni;
using namespace parseroni;

using JsonContext = parseroni::TextParseContext;
using JsonNode    = parseroni::TextParseNode;

//------------------------------------------------------------------------------
// Numbers

using sign     = Atoms<'+', '-'>;
using digit    = Range<'0', '9'>;
using onenine  = Range<'1', '9'>;
using digits   = Some<digit>;
using integer  = Seq<Opt<Atom<'-'>>, Oneof<Seq<onenine, digits>, digit>>;
using fraction = Seq<Atom<'.'>, digits>;
using exponent = Seq<Atoms<'e', 'E'>, Opt<sign>, digits>;
using number   = Seq<integer, Opt<fraction>, Opt<exponent>>;

//------------------------------------------------------------------------------
// Strings

using ws        = Any<Atoms<' ', '\n', '\r', '\t'>>;
using hex       = Ranges<'0','9','a','f','A','F'>;
using escape    = Seq<Atom<'\\'>, Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>>;
using character = Oneof<Seq<Not<Atom<'"'>>, Not<Atom<'\\'>>, Range<0x0020, 0x10FFFF>>, escape>;
using string    = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

//------------------------------------------------------------------------------
// Arrays

TextSpan match_value(JsonContext& ctx, TextSpan body);

template <typename pattern>
using list  = Seq<pattern, Any<Seq<ws, Atom<','>, ws, pattern>>>;
using value = Ref<match_value>;
using array = Seq<Atom<'['>, ws, Opt<list<value>>, ws, Atom<']'>>;

//------------------------------------------------------------------------------
// Objects

using key    = Capture<"key", string, JsonNode>;
using field  = Capture<"field", Seq<key, ws, Atom<':'>, ws, value>, JsonNode>;
using object = Seq<Atom<'{'>, ws, Opt<list<field>>, ws, Atom<'}'>>;
using json   = Seq<ws, value, ws>;

TextSpan match_value(JsonContext& ctx, TextSpan body) {
  using value =
  Capture<
    "val",
    Oneof<number, string, array, object, Lit<"true">, Lit<"false">, Lit<"null">>,
    JsonNode
  >;
  return value::match(ctx, body);
}

//------------------------------------------------------------------------------
// Parser

bool parse_json(const char* text, int size) {
  TextSpan body(text, text + size);

  static JsonContext ctx;
  ctx.reset();
  auto result = json::match(ctx, body);

#ifdef DEBUG
  utils::print_trees(ctx, body, 40, 0);
#endif

  return result.is_valid();
}

//------------------------------------------------------------------------------
