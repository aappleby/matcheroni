#include "c_lexer.h"
#include "Matcheroni.h"
#include <stdint.h>
#include <stdio.h>

using namespace matcheroni;

extern const char* current_filename;

//------------------------------------------------------------------------------

using utf8_ext       = Range<0x80, 0xBF>;
using utf8_onebyte   = Range<0x00, 0x7F>;
using utf8_twobyte   = Seq<Range<0xC0, 0xDF>, utf8_ext>;
using utf8_threebyte = Seq<Range<0xE0, 0xEF>, utf8_ext, utf8_ext>;
using utf8_fourbyte  = Seq<Range<0xF0, 0xF7>, utf8_ext, utf8_ext, utf8_ext>;

using utf8_char = Oneof<utf8_onebyte, utf8_twobyte, utf8_threebyte, utf8_fourbyte>;
using utf8_multi = Oneof<utf8_twobyte, utf8_threebyte, utf8_fourbyte>;
using utf8_bom = Seq<Char<0xEF>, Char<0xBB>, Char<0xBF>>;

const char* match_utf8(const char* text, void* ctx) {
  auto end = utf8_char::match(text, ctx);
  return end;
}

const char* match_utf8_bom(const char* text, void* ctx) {
  return utf8_bom::match(text, ctx);
}

// Not sure if this should be in here
using latin1_ext = Range<128,255>;

/*
// Yeaaaah, not gonna try to support trigraphs, they're obsolete and have been
// removed from the latest C spec.
using trigraphs = OneofLit<
  R"(??=)",
  R"(??()",
  R"(??/)",
  R"(??))",
  R"(??')",
  R"(??<)",
  R"(??!)",
  R"(??>)",
  R"(??-)"
>;
*/

//------------------------------------------------------------------------------
// Misc

const char* match_space(const char* text, void* ctx) {
  using ws = Char<' ','\t'>;
  using match = Some<ws>;
  return match::match(text, ctx);
}

const char* match_newline(const char* text, void* ctx) {
  using match = Seq<Opt<Char<'\r'>>, Char<'\n'>>;
  return match::match(text, ctx);
}

const char* match_indentation(const char* text, void* ctx) {
  using match = Seq<Char<'\n'>, Any<Char<' ', '\t'>>>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
// 5.1.1.2 : Lines ending in a backslash and a newline get spliced together
// with the following line.

// According to GCC it's only a warning to have whitespace between the
// backslash and the newline... and apparently \r\n is ok too?

using space = Char<' ','\t'>;
using splice = Seq<
  Char<'\\'>,
  Any<space>,
  Opt<Char<'\r'>>,
  Any<space>,
  Char<'\n'>
>;

const char* match_splice(const char* text, void* ctx) {
  return splice::match(text, ctx);
}

//------------------------------------------------------------------------------
// 6.4.1 Keywords

const char* match_keyword(const char* text, void* ctx) {
  const char* keywords[55] = {
    "alignas", "alignof", "auto", "bool", "break", "case", "char", "const",
    "constexpr", "continue", "default", "do", "double", "else", "enum",
    "extern", "false", "float", "for", "goto", "if", "inline", "int", "long",
    "nullptr", "register", "restrict", "return", "short", "signed", "sizeof",
    "static", "static_assert", "struct", "switch", "thread_local", "true",
    "typedef", "typeof", "typeof_unqual", "union", "unsigned", "void",
    "volatile", "while", "", "_Atomic", "_BitInt", "_Complex", "_Decimal128",
    "_Decimal32", "_Decimal64", "_Generic", "_Imaginary", "_Noreturn"
  };
  for (int i = 0; i < 55; i++) {
    const char* k = keywords[i];
    const char* t = text;
    while (*k && *t && (*k == *t)) {
      k++;
      t++;
    }
    if (*k == 0) return t;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
// 6.4.4.1 Integer constants

template<typename M>
using ticked = Seq<Opt<Char<'\''>>, M>;

using digit                = Range<'0', '9'>;
using nonzero_digit        = Range<'1', '9'>;
using decimal_constant     = Seq<nonzero_digit, Any<ticked<digit>>>;

using hexadecimal_prefix         = Oneof<Lit<"0x">, Lit<"0X">>;
using hexadecimal_digit          = Oneof<digit, Range<'a', 'f'>, Range<'A', 'F'>>;
using hexadecimal_digit_sequence = Seq<hexadecimal_digit, Any<ticked<hexadecimal_digit>>>;
using hexadecimal_constant       = Seq<hexadecimal_prefix, hexadecimal_digit_sequence>;

using binary_prefix         = Oneof<Lit<"0b">, Lit<"0B">>;
using binary_digit          = Char<'0','1'>;
using binary_digit_sequence = Seq<binary_digit, Any<ticked<binary_digit>>>;
using binary_constant       = Seq<binary_prefix, binary_digit_sequence>;

using octal_digit        = Range<'0', '7'>;
using octal_constant     = Seq<Char<'0'>, Any<ticked<octal_digit>>>;

using unsigned_suffix        = Char<'u', 'U'>;
using long_suffix            = Char<'l', 'L'>;
using long_long_suffix       = Oneof<Lit<"ll">, Lit<"LL">>;
using bit_precise_int_suffix = Oneof<Lit<"wb">, Lit<"WB">>;

using integer_suffix = Oneof<
  Seq<unsigned_suffix,  Opt<long_suffix>>,
  Seq<unsigned_suffix,  Opt<long_long_suffix>>,
  Seq<long_suffix,      Opt<unsigned_suffix>>,
  Seq<long_long_suffix, Opt<unsigned_suffix>>,

  Seq<unsigned_suffix, bit_precise_int_suffix>,
  Seq<bit_precise_int_suffix, Opt<unsigned_suffix>>
>;

// Octal has to be _after_ bin/hex so we don't prematurely match the prefix
using integer_constant = Oneof<
  Seq<decimal_constant,     Opt<integer_suffix>>,
  Seq<hexadecimal_constant, Opt<integer_suffix>>,
  Seq<binary_constant,      Opt<integer_suffix>>,
  Seq<octal_constant,       Opt<integer_suffix>>
>;

const char* match_int(const char* text, void* ctx) {
  return integer_constant::match(text, ctx);
}

//------------------------------------------------------------------------------
// 6.4.3 Universal character names

using n_char = NotChar<'}','\n'>;
using n_char_sequence = Some<n_char>;
using named_universal_character = Seq<Lit<"\\N{">, n_char_sequence, Lit<"}">>;

using hex_quad = Rep<4, hexadecimal_digit>;

using universal_character_name = Oneof<
  Seq< Lit<"\\u">, hex_quad >,
  Seq< Lit<"\\U">, hex_quad, hex_quad >,
  Seq< Lit<"\\u{">, Any<hexadecimal_digit>, Lit<"}">>,
  named_universal_character
>;

//------------------------------------------------------------------------------
// 6.4.2 Identifiers - GCC allows dollar signs in identifiers?

using nondigit = Oneof<
  Range<'a', 'z'>,
  Range<'A', 'Z'>,
  Char<'_'>,
  Char<'$'>,
  universal_character_name,
  latin1_ext, // One of the GCC test files requires this
  utf8_multi  // Lots of GCC test files for unicode
>;

using identifier = Seq<nondigit, Any<Oneof<digit, nondigit>>>;

const char* match_identifier(const char* text, void* ctx) {
  return identifier::match(text, ctx);
}

//------------------------------------------------------------------------------
// 6.4.4.2 Floating constants

using floating_suffix = Oneof<
  Char<'f'>, Char<'l'>, Char<'F'>, Char<'L'>,
  Lit<"df">, Lit<"dd">, Lit<"dl">,
  Lit<"DF">, Lit<"DD">, Lit<"DL">
>;

using ticked_digit   = Seq<Opt<Char<'\''>>, digit>;
using digit_sequence = Seq<digit, Any<ticked_digit>>;

using fractional_constant = Oneof<
  Seq<Opt<digit_sequence>, Char<'.'>, digit_sequence>,
  Seq<digit_sequence, Char<'.'>>
>;

using sign = Char<'+','-'>;

using hexadecimal_fractional_constant = Oneof<
  Seq<Opt<hexadecimal_digit_sequence>, Char<'.'>, hexadecimal_digit_sequence>,
  Seq<hexadecimal_digit_sequence, Char<'.'>>
>;

using exponent_part        = Seq<Char<'e', 'E'>, Opt<sign>, digit_sequence>;
using binary_exponent_part = Seq<Char<'p', 'P'>, Opt<sign>, digit_sequence>;

using decimal_floating_constant = Oneof<
  Seq< fractional_constant, Opt<exponent_part>, Opt<floating_suffix> >,
  Seq< digit_sequence, exponent_part, Opt<floating_suffix> >
>;

using hexadecimal_floating_constant = Seq<
  hexadecimal_prefix,
  Oneof<hexadecimal_fractional_constant, hexadecimal_digit_sequence>,
  binary_exponent_part,
  Opt<floating_suffix>
>;

using floating_constant = Oneof<
  decimal_floating_constant,
  hexadecimal_floating_constant
>;

const char* match_float(const char* text, void* ctx) {
  return floating_constant::match(text, ctx);
}

//------------------------------------------------------------------------------
// 6.4.4.3 Enumeration constants

using enumeration_constant = identifier;

//------------------------------------------------------------------------------
// 6.4.4.4 Character constants


//using simple_escape_sequence      = Seq<Char<'\\'>, Charset<"'\"?\\abfnrtv">>;

// GCC adds \e and \E, and '\(' '\{' '\[' '\%' are warnings but allowed
using simple_escape_sequence      = Seq<Char<'\\'>, Charset<"'\"?\\abfnrtveE({[%">>;

using octal_escape_sequence = Oneof<
  Seq<Char<'\\'>, octal_digit, Opt<octal_digit>, Opt<octal_digit>>,
  Seq<Lit<"\\o{">, Any<octal_digit>, Lit<"}">>
>;

using hexadecimal_escape_sequence = Oneof<
  Seq<Lit<"\\x">, Some<hexadecimal_digit>>,
  Seq<Lit<"\\x{">, Any<hexadecimal_digit>, Lit<"}">>
>;

using escape_sequence = Oneof<
  simple_escape_sequence,
  octal_escape_sequence,
  hexadecimal_escape_sequence,
  universal_character_name
>;

const char* match_escape_sequence(const char* text, void* ctx) {
  return escape_sequence::match(text, ctx);
}

// Multi-character character literals are allowed by spec, but their meaning is
// implementation-defined...

using encoding_prefix    = Oneof<Lit<"u8">, Char<'u', 'U', 'L'>>; // u8 must go first
using c_char             = Oneof<escape_sequence, NotChar<'\'', '\\', '\n'>>;
using c_char_sequence    = Some<c_char>;

//using character_constant = Seq< Opt<encoding_prefix>, Char<'\''>, c_char_sequence, Char<'\''> >;

// GCC allows empty character constants?
using character_constant = Seq< Opt<encoding_prefix>, Char<'\''>, Any<c_char>, Char<'\''> >;

const char* match_character_constant(const char* text, void* ctx) {
  return character_constant::match(text, ctx);
}

//------------------------------------------------------------------------------
// 6.4.4.5 Predefined constants

using predefined_constant = Oneof<Lit<"false">, Lit<"true">, Lit<"nullptr">>;

//------------------------------------------------------------------------------
// 6.4.5 String literals

// Note, we add splices here since we're matching before preproc
using s_char          = Oneof<splice, escape_sequence, NotChar<'"', '\\', '\n'>>;
using s_char_sequence = Some<s_char>;
using string_literal  = Seq<Opt<encoding_prefix>, Char<'"'>, Opt<s_char_sequence>, Char<'"'>>;

// Raw string literals from the C++ spec
// We ignore backslash in d_char for similar splice-related reasons
//using d_char          = NotChar<' ', '(', ')', '\\', '\t', '\v', '\f', '\n'>;
using d_char          = NotChar<' ', '(', ')', '\t', '\v', '\f', '\n'>;
using d_char_sequence = Some<d_char>;

using r_terminator    = Seq<Char<')'>, MatchBackref, Char<'"'>>;
using r_char          = Seq<Not<r_terminator>, Char<>>;
using r_char_sequence = Some<r_char>;

using raw_string_literal = Seq<
  Opt<encoding_prefix>,
  Char<'R'>,
  Char<'"'>,
  StoreBackref<Opt<d_char_sequence>>,
  Char<'('>,
  Opt<r_char_sequence>,
  Char<')'>,
  MatchBackref,
  Char<'"'>
>;

const char* match_string_literal(const char* text, void* ctx) {
  return string_literal::match(text, ctx);
}

const char* match_raw_string_literal(const char* text, void* ctx) {
  // Raw strings require matching backreferences, so we squish that into the
  // grammar by passing the backreference in the context pointer.
  Backref backref;
  return raw_string_literal::match(text, &backref);
}

//------------------------------------------------------------------------------
// 6.4.6 Punctuators
/*
[ ] ( ) { } . ->
++ -- & * + - ~ !
/ % << >> < > <= >= == != ^ | && ||
? : :: ; ...
= *= /= %= += -= <<= >>= &= ^= |=
, # ##
<: :> <% %> %: %:%:
*/

#if 0
using trigraphs = Trigraphs<"...<<=>>=">;
using digraphs  = Digraphs<"---=->!=*=/=&&&=##%=^=+++=<<<===>=>>|=||">;
using unigraphs = Charset<"-,;:!?.()[]{}*/&#%^+<=>|~">;
using all_punct = Oneof<trigraphs, digraphs, unigraphs>;
#endif

// We're just gonna match these one punct at a time
using punctuator = Charset<"-,;:!?.()[]{}*/&#%^+<=>|~">;

const char* match_punct(const char* text, void* ctx) {

  //return all_punct::match(text, ctx);
  return punctuator::match(text, ctx);
}

//------------------------------------------------------------------------------
// 6.4.7 Header names

using q_char          = NotChar<'\n', '"'>;
using q_char_sequence = Some<q_char>;
using h_char          = NotChar<'\n', '>'>;
using h_char_sequence = Some<h_char>;
using header_name = Oneof<
  Seq< Char<'<'>, h_char_sequence, Char<'>'> >,
  Seq< Char<'"'>, q_char_sequence, Char<'"'> >
>;

//------------------------------------------------------------------------------
// 6.4.9 Comments

// Single-line comments
const char* match_oneline_comment(const char* text, void* ctx) {
  using match = Seq<Lit<"//">, Until<EOL>>;
  return match::match(text, ctx);
}

// Multi-line non-nested comments
const char* match_multiline_comment(const char* text, void* ctx) {
  // not-nested version
  using ldelim = Lit<"/*">;
  using rdelim = Lit<"*/">;
  using match  = Seq<ldelim, Until<rdelim>, rdelim>;
  return match::match(text, ctx);
}

// Multi-line nested comments (not actually in C, just here for reference)
// FIXME this can be singly-recursive

const char* match_nested_comment_body(const char* text, void* ctx);

const char* match_nested_comment(const char* text, void* ctx) {
  using ldelim = Lit<"/*">;
  using rdelim = Lit<"*/">;
  using item   = Seq<Not<ldelim>, Not<rdelim>, Char<>>;
  using body   = Ref<match_nested_comment_body>;
  using match  = Seq<ldelim, body, rdelim>;
  return match::match(text, ctx);
}

const char* match_nested_comment_body(const char* text, void* ctx) {
  using ldelim = Lit<"/*">;
  using rdelim = Lit<"*/">;
  using item   = Seq<Not<ldelim>, Not<rdelim>, Char<>>;
  using nested = Ref<match_nested_comment>;
  using match  = Any<Oneof<nested,item>>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
// We can't really handle preprocessing stuff so we're just matching the
// initial keyword.

using preproc = Seq<
  Char<'#'>,
  OneofLit<
    "define", "elif", "elifdef", "elifndef", "else", "embed", "endif", "error",
    "if", "ifdef", "ifndef", "include", "line", "pragma", "undef", "warning"
  >
>;

const char* match_preproc(const char* text, void* ctx) {
  return preproc::match(text, ctx);
}

//------------------------------------------------------------------------------
