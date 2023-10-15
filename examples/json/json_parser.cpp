// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "json.hpp"

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

TextSpan match_value(JsonParseContext& ctx, TextSpan body);
using value  = Ref<match_value>;
using array  = Seq<Atom<'['>, ws, Opt<list<value>>, ws, Atom<']'>>;
using key    = Capture<"key", string, JsonString>;
using member = Capture<"member", Seq<key, ws, Atom<':'>, ws, value>, JsonKeyVal>;
using object = Seq<Atom<'{'>, ws, Opt<list<member>>, ws, Atom<'}'>>;

TextSpan match_value(JsonParseContext& ctx, TextSpan body) {
  using value = Oneof<
    Capture<"val", string,       JsonString>,
    Capture<"val", number,       JsonNumber>,
    Capture<"val", array,        JsonArray>,
    Capture<"val", object,       JsonObject>,
    Capture<"val", Lit<"true">,  JsonKeyword>,
    Capture<"val", Lit<"false">, JsonKeyword>,
    Capture<"val", Lit<"null">,  JsonKeyword>
  >;
  return value::match(ctx, body);
}

using json = Seq<ws, value, ws>;

TextSpan parse_json(JsonParseContext& ctx, TextSpan body) {
  return json::match(ctx, body);
}
