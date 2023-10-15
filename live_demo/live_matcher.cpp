#include "live_json.hpp"

using namespace matcheroni;
using namespace parseroni;

using sign      = Atom<'+', '-'>;
using digit     = Range<'0', '9'>;
using onenine   = Range<'1', '9'>;
using digits    = Some<digit>;
using integer   = Seq<Opt<Atom<'-'>>, Oneof<Seq<onenine, digits>, digit>>;
using fraction  = Seq<Atom<'.'>, digits>;
using exponent  = Seq<Atom<'e', 'E'>, Opt<sign>, digits>;
using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;

using ws        = Any<Atom<' ', '\n', '\r', '\t'>>;
using hex       = Range<'0','9','a','f','A','F'>;
using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
using character = Oneof<
  Seq<Not<Atom<'"'>>, Not<Atom<'\\'>>, Range<0x0020, 0x10FFFF>>,
  Seq<Atom<'\\'>, escape>
>;
using string = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

template <typename P>
using list = Seq<P, Any<Seq<ws, Atom<','>, ws, P>>>;

static TextSpan match_value(JsonMatchContext& ctx, TextSpan body);
using value  = Ref<match_value>;
using array  = Seq<Atom<'['>, ws, Opt<list<value>>, ws, Atom<']'>>;
using key    = string;
using member = Seq<key, ws, Atom<':'>, ws, value>;
using object = Seq<Atom<'{'>, ws, Opt<list<member>>, ws, Atom<'}'>>;

static TextSpan match_value(JsonMatchContext& ctx, TextSpan body) {
  using value = Oneof<number, string, array, object, Lit<"true">, Lit<"false">, Lit<"null">>;
  return value::match(ctx, body);
}

using json = Seq<ws, value, ws>;

TextSpan match_json(JsonMatchContext& ctx, TextSpan body) {
  return json::match(ctx, body);
}
