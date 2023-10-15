#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"
#include <string>

using namespace matcheroni;
using namespace parseroni;

//------------------------------------------------------------------------------

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

struct JsonContext : public parseroni::NodeContext<JsonNode, true, true> {
  static int atom_cmp(char a, int b) { return (unsigned char)a - b; }
};

//------------------------------------------------------------------------------

using sign     = Atom<'+', '-'>;
using digit    = Range<'0', '9'>;
using onenine  = Range<'1', '9'>;
using digits   = Some<digit>;
using integer  = Seq<Opt<Atom<'-'>>, Oneof<Seq<onenine, digits>, digit>>;
using fraction = Seq<Atom<'.'>, digits>;
using exponent = Seq<Atom<'e', 'E'>, Opt<sign>, digits>;
using number   = Seq<integer, Opt<fraction>, Opt<exponent>>;

//------------------------------------------------------------------------------

using ws = Any<Atom<' ', '\n', '\r', '\t'>>;
using hex = Range<'0','9','a','f','A','F'>;

using escape = Seq<Atom<'\\'>, Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>>;

using character = Oneof<Seq<Not<Atom<'"'>>, Not<Atom<'\\'>>, Range<0x0020, 0x10FFFF>>, escape>;

using string = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

//------------------------------------------------------------------------------

template <typename pattern>
using list = Seq<pattern, Any<Seq<ws, Atom<','>, ws, pattern>>>;

TextSpan match_value(JsonContext& ctx, TextSpan body);
using value  = Ref<match_value>;

using array = Seq<Atom<'['>, ws, Opt<list<value>>, ws, Atom<']'>>;

//------------------------------------------------------------------------------

using key    = Capture<"key", string, JsonString>;
using member = Capture<"member", Seq<key, ws, Atom<':'>, ws, value>, JsonKeyVal>;
using object = Seq<Atom<'{'>, ws, Opt<list<member>>, ws, Atom<'}'>>;

//------------------------------------------------------------------------------

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

using json = Seq<ws, value, ws>;

//------------------------------------------------------------------------------

bool parse_json(const std::string& text, bool verbose) {
  TextSpan body(text.data(), text.data() + text.size());

  static JsonContext ctx;
  ctx.reset();
  auto result = json::match(ctx, body);

  if (verbose) {
    if (text.size() < 1024) utils::print_trees(ctx, body, 40, 0);
    //printf("Parse nodes: %d, ", ctx.alloc.alloc_count);
  }

  return result.is_valid();
}

//------------------------------------------------------------------------------
