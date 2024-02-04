#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"

#ifdef DEBUG
#include "matcheroni/Utilities.hpp"
#endif

using namespace matcheroni;
using namespace parseroni;

//------------------------------------------------------------------------------
// Custom node types

struct JsonNode : public parseroni::NodeBase<JsonNode, char> {
  virtual ~JsonNode() {}
  matcheroni::TextSpan as_text_span() const { return span; }
};

struct JsonNumber  : public JsonNode {};
struct JsonString  : public JsonNode {};
struct JsonArray   : public JsonNode {};
struct JsonKeyVal  : public JsonNode {};
struct JsonObject  : public JsonNode {};
struct JsonKeyword : public JsonNode {};

struct JsonContext : public parseroni::NodeContext<JsonNode> {
  static int atom_cmp(char a, int b) { return (unsigned char)a - b; }
};

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

using key    = Capture<"key", string, JsonString>;
using field  = Capture<"field", Seq<key, ws, Atom<':'>, ws, value>, JsonKeyVal>;
using object = Seq<Atom<'{'>, ws, Opt<list<field>>, ws, Atom<'}'>>;
using json   = Seq<ws, value, ws>;

TextSpan match_value(JsonContext& ctx, TextSpan body) {
  using value =
  Oneof<
    Capture<"val", number,       JsonNumber>,
    Capture<"val", string,       JsonString>,
    Capture<"val", array,        JsonArray>,
    Capture<"val", object,       JsonObject>,
    Capture<"val", Lit<"true">,  JsonKeyword>,
    Capture<"val", Lit<"false">, JsonKeyword>,
    Capture<"val", Lit<"null">,  JsonKeyword>
  >;
  return value::match(ctx, body);
}

//------------------------------------------------------------------------------
// Parser

bool parse_json(const char* text, int size) {
  static JsonContext ctx;
  TextSpan body(text, text + size);

  auto result = json::match(ctx, body);

#ifdef DEBUG
  utils::print_trees(ctx, body, 40, 0);
#endif

  return result.is_valid();
}

//------------------------------------------------------------------------------
