#include "Matcheroni.hpp"

using namespace matcheroni;

using hex = Oneof<Range<'0','9'>, Range<'A','F'>, Range<'a','f'>>;
using escape =
Oneof<
  Atom<'"','\\','/','b','f','n','r','t'>,
  Seq<Atom<'u'>, Rep<4, hex>>
>;

using ws        = Any<Atom<' ','\n','\r','\t'>>;
using keyword   = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;
using character = Oneof< NotAtom<'"','\\'>, Seq<Atom<'\\'>, escape> >;
using string    = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

using sign     = Atom<'+','-'>;
using digit    = Range<'0','9'>;
using onenine  = Range<'1','9'>;
using digits   = Some<digit>;
using fraction = Seq<Atom<'.'>, digits>;
using exponent = Seq<Atom<'e','E'>, Opt<sign>, digits>;
using integer  = Seq< Opt<Atom<'-'>>, Oneof<digit,Seq<onenine,digits>> >;
using number   = Seq<integer, Opt<fraction>, Opt<exponent>>;

const char* match_value(void* ctx, const char* a, const char* b);
using value = Ref<match_value>;

using element  = Seq<ws, value, ws>;
using elements = Seq<element, Any<Seq<Atom<','>, element>> >;
using member   = Seq<ws, string, ws, Atom<':'>, element>;
using members  = Seq<member, Any<Seq<Atom<','>, member>> >;
using object   = Seq<Atom<'{'>, Oneof<ws, members>,  Atom<'}'>>;
using array    = Seq<Atom<'['>, Oneof<ws, elements>, Atom<']'>>;

const char* match_value(void* ctx, const char* a, const char* b) {
  using value = Oneof<object, array, string, number, keyword>;
  return value::match(ctx, a, b);
}

using json = element;
