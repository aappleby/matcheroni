#include "matcheroni/Matcheroni.hpp"

using namespace matcheroni;

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

using hex       = Range<'0','9','a','f','A','F'>;
using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
using character = Oneof<
  Seq<Not<Atom<'"'>>, Not<Atom<'\\'>>, Range<0x0020, 0x10FFFF>>,
  Seq<Atom<'\\'>, escape>
>;

using string = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

//------------------------------------------------------------------------------

using ws  = Some<Atom<' ', '\n', '\r', '\t'>>;

template <typename P>
using list = Seq<P, Any<Seq<Opt<ws>, Atom<','>, Opt<ws>, P>>>;

//TextSpan match_value(TextMatchContext& ctx, TextSpan body);
//using value  = Ref<match_value>;

// FIXME use delimited_list or something

template<typename ldelim, typename pattern, typename rdelim>
using delimited_list = Seq<ldelim, Opt<ws>, comma_separated<pattern>, Opt<ws>, rdelim>;

using kvpair  = Seq<string, Opt<ws>, Atom<':'>, Opt<ws>, value>;

using array   = Seq<Atom<'['>, Opt<ws>, Opt<list<value>>, Opt<ws>, Atom<']'>>;
using object  = Seq<Atom<'{'>, Opt<ws>, Opt<list<kvpair>>, Opt<ws>, Atom<'}'>>;

using value   = Oneof<number, string, array, object, Lit<"true">, Lit<"false">, Lit<"null">>;
using json    = Seq<Opt<ws>, value, Opt<ws>>;

//------------------------------------------------------------------------------

//TextSpan match_value(TextMatchContext& ctx, TextSpan body) {
//  return any::match(ctx, body);
//}

//------------------------------------------------------------------------------

TextSpan match_json(TextMatchContext& ctx, TextSpan body) {
  return json::match(ctx, body);
}

//------------------------------------------------------------------------------
