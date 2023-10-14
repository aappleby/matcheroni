// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "json_matcher.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;


// Matches any JSON number
using sign      = Atom<'+', '-'>;
using digit     = Range<'0', '9'>;
using onenine   = Range<'1', '9'>;
using digits    = Some<digit>;
using integer   = Seq<Opt<Atom<'-'>>, Oneof<Seq<onenine, digits>, digit>>;
using fraction  = Seq<Atom<'.'>, digits>;
using exponent  = Seq<Atom<'e', 'E'>, Opt<sign>, digits>;
using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;

// Matches a JSON string that can contain valid escape characters
using ws        = Any<Atom<' ', '\n', '\r', '\t'>>;
using hex       = Range<'0','9','a','f','A','F'>;
using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
using character = Oneof<
  Seq<Not<Atom<'"'>>, Not<Atom<'\\'>>, Range<0x0020, 0x10FFFF>>,
  Seq<Atom<'\\'>, escape>
>;
using string = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

// Matches a comma-delimited list with embedded whitespace
template <typename P>
using list  = Seq<P, Any<Seq<ws, Atom<','>, ws, P>>>;

// Matches any valid JSON value
static TextSpan match_value(TextMatchContext& ctx, TextSpan body);
using value  = Ref<match_value>;

// Matches bracket-delimited lists of JSON values
using array  = Seq<Atom<'['>, ws, Opt<list<value>>, ws, Atom<']'>>;

// Matches a key:val pair where 'key' is a string and 'val' is any JSON value.
using key    = string;
using member = Seq<key, ws, Atom<':'>, ws, value>;

// Matches a curly-brace-delimited list of key:val pairs.
using object = Seq<Atom<'{'>, ws, Opt<list<member>>, ws, Atom<'}'>>;

static TextSpan match_value(TextMatchContext& ctx, TextSpan body) {
  using value = Oneof<number, string, array, object, Lit<"true">, Lit<"false">, Lit<"null">>;
  return value::match(ctx, body);
}

// Matches any valid JSON document
using json = Seq<ws, value, ws>;

__attribute__((noinline))
TextSpan match_json(TextMatchContext& ctx, TextSpan body) {
  return json::match(ctx, body);
}
