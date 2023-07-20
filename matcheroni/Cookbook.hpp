#pragma once
#include "matcheroni/Matcheroni.hpp"

namespace matcheroni {
namespace cookbook {

  //----------------------------------------
  // Basic text

  using space = Any<Atom<' ','\n','\r','\t'>>;

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

  //----------------------------------------
  // Delimited spans

  template<typename ldelim, typename rdelim>
  using delimited_span = Seq<ldelim, Until<rdelim>, rdelim>;

  using dquote_span  = delimited_span<Atom<'"'>,  Atom<'"'>>;   // note - no \" support
  using squote_span  = delimited_span<Atom<'\''>, Atom<'\''>>;
  using bracket_span = delimited_span<Atom<'['>,  Atom<']'>>;
  using brace_span   = delimited_span<Atom<'{'>,  Atom<'}'>>;
  using paren_span   = delimited_span<Atom<'('>,  Atom<')'>>;

  // Python's triple-double-quoted string literal
  using triple_quote = Lit<R"(""")">;
  using multiline_string = delimited_span<triple_quote, triple_quote>;

  //----------------------------------------
  // Delimited lists

  template<typename pattern>
  using comma_separated = Seq<pattern, Any<Seq<space, Atom<','>, space, pattern>>>;

  template<typename ldelim, typename pattern, typename rdelim>
  using delimited_list =
  Seq<ldelim, space, comma_separated<pattern>, space, rdelim>;

  template<typename pattern> using paren_list   = delimited_list<Atom<'('>, pattern, Atom<')'>>;
  template<typename pattern> using bracket_list = delimited_list<Atom<'['>, pattern, Atom<']'>>;
};
};
