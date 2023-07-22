#pragma once
#include "matcheroni/Matcheroni.hpp"

namespace matcheroni {
namespace cookbook {

//----------------------------------------
// Character types from ctype.h

using isalnum = Range<'0','9','a','z','A','Z'>;
using isalpha = Range<'a', 'z', 'A', 'Z'>;
using isblank = Atom<' ', '\t'>;
using iscntrl = Range<0x00, 0x1F, 0x7F, 0x7F>;
using isdigit = Range<'0', '9'>;
using isgraph = Range<0x21, 0x7E>;
using islower = Range<'a', 'z'>;
using isprint = Range<0x20, 0x7E>;
using ispunct = Range<0x21, 0x2F, 0x3A, 0x40, 0x5B, 0x60, 0x7B, 0x7E>;
using isspace = Atom<' ','\f','\v','\n','\r','\t'>;
using isupper = Range<'A', 'Z'>;
using isxdigit = Range<'0','9','a','f','A','F'>;

//----------------------------------------
// Numbers

using sign = Atom<'+','-'>;

using decimal_digit   = Range<'0','9'>;
using decimal_nonzero = Range<'1','9'>;
using decimal_digits  = Some<decimal_digit>;
using decimal_integer = Seq< Opt<sign>, Oneof< Seq<decimal_nonzero,decimal_digits>, decimal_digit> >;

using decimal_fraction = Seq<Atom<'.'>, decimal_digits>;
using decimal_exponent = Seq<Atom<'e','E'>, Opt<sign>, decimal_digits>;
using decimal_float    = Seq<decimal_integer, Opt<decimal_fraction>, Opt<decimal_exponent>>;

using hexadecimal_prefix  = Oneof<Lit<"0x">, Lit<"0X">>;
using hexadecimal_digit   = Range<'0','9','a','f','A','F'>;
using hexadecimal_digits  = Some<hexadecimal_digit>;
using hexadecimal_integer = Seq< Opt<sign>, hexadecimal_prefix, hexadecimal_digits >;

//----------------------------------------
// Delimited spans

template<typename ldelim, typename rdelim>
using delimited_span = Seq<ldelim, Until<rdelim>, rdelim>;

using dquote_span  = delimited_span<Atom<'"'>,  Atom<'"'>>;   // note - no \" support
using squote_span  = delimited_span<Atom<'\''>, Atom<'\''>>;  // note - no \' support
using bracket_span = delimited_span<Atom<'['>,  Atom<']'>>;
using brace_span   = delimited_span<Atom<'{'>,  Atom<'}'>>;
using paren_span   = delimited_span<Atom<'('>,  Atom<')'>>;

// Python's triple-double-quoted string literal
using triple_quote = Lit<R"(""")">;
using multiline_string = delimited_span<triple_quote, triple_quote>;

//----------------------------------------
// Separated = a,   b , c   , d

template<typename pattern, typename delim>
using separated =
Seq<
  pattern,
  Any<Seq<Any<isspace>, delim, Any<isspace>, pattern>>,
  // trailing delimiter OK
  Opt<Seq<Any<isspace>, delim>>
>;

template<typename pattern>
using comma_separated = separated<pattern, Atom<','>>;

//----------------------------------------
// Joined = a.b.c.d

template<typename pattern, typename delim>
using joined =
Seq<
  pattern,
  Any<Seq<delim, pattern>>
  // trailing delimiter _not_ OK
>;

template<typename pattern>
using dot_joined = joined<pattern, Atom<'.'>>;

//----------------------------------------
// Delimited lists

template<typename ldelim, typename pattern, typename rdelim>
using delimited_list = Seq<ldelim, Any<isspace>, comma_separated<pattern>, Any<isspace>, rdelim>;

template<typename pattern> using paren_list   = delimited_list<Atom<'('>, pattern, Atom<')'>>;
template<typename pattern> using bracket_list = delimited_list<Atom<'['>, pattern, Atom<']'>>;
template<typename pattern> using brace_list   = delimited_list<Atom<'{'>, pattern, Atom<'}'>>;

//----------------------------------------
// Basic UTF8

using utf8_ext       = Range<0x80, 0xBF>;
using utf8_onebyte   = Range<0x00, 0x7F>;
using utf8_twobyte   = Seq<Range<0xC0, 0xDF>, utf8_ext>;
using utf8_threebyte = Seq<Range<0xE0, 0xEF>, utf8_ext, utf8_ext>;
using utf8_fourbyte  = Seq<Range<0xF0, 0xF7>, utf8_ext, utf8_ext, utf8_ext>;
using utf8_bom       = Seq<Atom<0xEF>, Atom<0xBB>, Atom<0xBF>>;

}; // namespace cookbook
}; // namespace matcheroni
