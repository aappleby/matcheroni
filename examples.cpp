#include "examples.h"
#include "Matcheroni.h"
#include <stdint.h>
#include <stdio.h>

using namespace matcheroni;

// FIXME bit-precise suffixes

//------------------------------------------------------------------------------
// 6.4.2 Identifiers - GCC allows dollar signs in identifiers?
// 6.4.2.1 General

using digit = Range<'0', '9'>;
using nondigit = Oneof<Range<'a', 'z'>, Range<'A', 'Z'>, Char<'_'>, Char<'$'>>;
using identifier = Seq<nondigit, Any<Oneof<digit, nondigit>>>;

const char* match_identifier(const char* text, void* ctx) {
  using lower = Range<'a', 'z'>;
  using upper = Range<'A', 'Z'>;
  using item  = Oneof<lower, upper, digit, Char<'_'>, Char<'$'>>;
  using match = Seq<Not<digit>, Some<item>>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
// Misc

using nonzero_digit = Range<'1', '9'>;

const char* match_byte_order_mark(const char* text, void* ctx) {
  using bom = Seq<Char<0xEF>, Char<0xBB>, Char<0xBF>>;
  return bom::match(text, ctx);
}

const char* match_utf8(const char* text, void* ctx) {
  using ext       = Range<0x80, 0xBF>;
  using onebyte   = Range<0x00, 0x7F>;
  using twobyte   = Seq<Range<0xC0, 0xDF>, ext>;
  using threebyte = Seq<Range<0xE0, 0xEF>, ext, ext>;
  using fourbyte  = Seq<Range<0xF0, 0xF7>, ext, ext, ext>;

  using match = Oneof<onebyte, twobyte, threebyte, fourbyte>;
  return match::match(text, ctx);
}

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

const char* match_ws(const char* text, void* ctx) {
  using ws = Char<' ','\t','\r','\n'>;
  return ws::match(text, ctx);
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
// 6.4   Lexical Elements
/*
using token = Oneof<
  keyword,
  identifier,
  constant,
  string_literal,
  punctuator
>;

preprocessing-token:
  header-name
  identifier
  pp-number
  character-constant
  string-literal
  punctuator
  each universal-character-name that cannot be one of the above
  each non-white-space character that cannot be one of the above
*/

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

using nonzero_digit = Range<'1', '9'>;
using octal_digit   = Range<'0', '7'>;
using binary_digit  = Char<'0','1'>;

using hexadecimal_digit          = Oneof<digit, Range<'a', 'f'>, Range<'A', 'F'>>;
using ticked_hexadecimal_digit   = Seq<Opt<Char<'\''>>, hexadecimal_digit>;
using hexadecimal_digit_sequence = Seq<hexadecimal_digit, Any<ticked_hexadecimal_digit>>;

/*
integer-constant:
  decimal-constant integer-suffixopt
  octal-constant integer-suffixopt
  hexadecimal-constant integer-suffixopt
  binary-constant integer-suffixopt

decimal-constant:
  nonzero-digit
  decimal-constant ’opt digit

octal-constant:
  0
  octal-constant ’opt octal-digit

hexadecimal-constant:
  hexadecimal-prefix hexadecimal-digit-sequence

binary-constant:
  binary-prefix binary-digit
  binary-constant Opt<Char<'\'’> binary-digit

using hexadecimal_prefix = Oneof<Lit<"0x">, Lit<"0X>>;
using binary_prefix = Oneof<Lit<"0b">, Lit<"0B">>;


integer-suffix:
  unsigned-suffix Opt<long-suffix>
  unsigned-suffix long-long-suffix
  unsigned-suffix bit-precise-int-suffix
  long-suffix Opt<unsigned-suffix>
  long-long-suffix Opt<unsigned-suffix>
  bit-precise-int-suffix Opt<unsigned-suffix>

bit-precise-int-suffix: one of
  wb WB

unsigned-suffix: one of
  u U

long-suffix: one of
  l L

long-long-suffix: one of
  ll LL
*/

// FIXME check grammar


const char* match_int(const char* text, void* ctx) {
  using tick       = Char<'\''>;

  using bin_prefix = Oneof<Lit<"0b">, Lit<"0B">>;
  using oct_prefix = Char<'0'>;
  using dec_prefix = Range<'1','9'>;
  using hex_prefix = Oneof<Lit<"0x">, Lit<"0X">>;

  using bin_digit  = Range<'0','1'>;
  using oct_digit  = Range<'0','7'>;
  using dec_digit  = Range<'0','9'>;
  using hex_digit  = Oneof<Range<'0','9'>, Range<'a','f'>, Range<'A','F'>>;

  // All digits after the first one can be prefixed by an optional '.
  using bin_digit2 = Seq<Opt<tick>, bin_digit>;
  using oct_digit2 = Seq<Opt<tick>, oct_digit>;
  using dec_digit2 = Seq<Opt<tick>, dec_digit>;
  using hex_digit2 = Seq<Opt<tick>, hex_digit>;

  using match_bin  = Seq<bin_prefix, bin_digit, Any<bin_digit2>>;
  using match_oct  = Seq<oct_prefix, oct_digit, Any<oct_digit2>>;
  using match_dec  = Seq<dec_prefix,            Any<dec_digit2>>;
  using match_hex  = Seq<hex_prefix, hex_digit, Any<hex_digit2>>;

  using suffix     = Oneof< Lit<"ul">, Lit<"lu">, Lit<"u">, Lit<"l"> >;

  // Octal has to be _after_ bin/hex so we don't prematurely match the prefix
  // Bare 0 has to be last for similar reasons.

  using match = Seq<
    Opt<Char<'-'>>,
    Oneof<
      match_bin,
      match_hex,
      match_oct,
      match_dec,
      Char<'0'>
    >,
    Opt<suffix>
  >;

  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
// 6.4.3 Universal character names
// FIXME check grammar

using hex_quad      = Rep<4, hexadecimal_digit>;

using universal_character_name = Oneof<
  Seq< Lit<"\\u">, hex_quad >,
  Seq< Lit<"\\U">, hex_quad, hex_quad >
>;

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

using hexadecimal_prefix = Oneof<Lit<"0x">, Lit<"0X">>;

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
  using digit  = Range<'0','9'>;
  using point  = Char<'.'>;
  using sign   = Char<'+','-'>;
  using exp    = Seq<Char<'e','E'>, Opt<sign>, Some<digit>>;
  using suffix = Char<'f','l','F','L'>;

  using with_dot = Seq<Any<digit>, point, Some<digit>, Opt<exp>, Opt<suffix>>;
  using no_dot   = Seq<Some<digit>, exp, Opt<suffix>>;

  using match = Oneof<with_dot, no_dot>;

  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
// 6.4.4.3 Enumeration constants

using enumeration_constant = identifier;

//------------------------------------------------------------------------------
// 6.4.4.4 Character constants

/*
character-constant:
  encoding-prefixopt ’ c-char-sequence ’

encoding-prefix: one of
  u8 u U L

c-char-sequence:
  c-char
  c-char-sequence c-char

c-char:
  any member of the source character set except the single-quote ’, backslash \, or new-line character
  escape-sequence

escape-sequence:
  simple-escape-sequence
  octal-escape-sequence
  hexadecimal-escape-sequence
  universal-character-name

using simple_escape_sequence = Seq<Char<'\\'>, Charset<"’\"?\\abfnrtv">>

using octal_escape_sequence = Seq<Char<'\\'>, octal_digit, Opt<octal_digit>, Opt<octal_digit>>

using hexadecimal_escape_sequence = Seq<Lit<"\x">, Some<hexadecimal_digit>>


*/

using simple_escape = Seq<Char<'\\'>, Charset<"'\"?\\abfnrtv">>;
using hex_escape    = Seq< Lit<"\\x">, Some<hexadecimal_digit> >;
using octal_escape  = Seq< Char<'\\'>, octal_digit, Opt<octal_digit>, Opt<octal_digit> >;

using escape_sequence = Oneof<
  simple_escape,
  octal_escape,
  hex_escape,
  universal_character_name
>;

const char* match_escape_sequence(const char* text, void* ctx) {
  return escape_sequence::match(text, ctx);
}

// Multi-character character literals are allowed by spec? But
// implementation-defined...

// FIXME grammar

const char* match_char_literal(const char* text, void* ctx) {
  using tick = Char<'\''>;
  using match = Seq<tick, Some<Oneof<escape_sequence, NotChar<'\''>>>, tick>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
// 6.4.4.5 Predefined constants

using predefined_constant = Oneof<Lit<"false">, Lit<"true">, Lit<"nullptr">>;

//------------------------------------------------------------------------------
// 6.4.5 String literals

using encoding_prefix  = Oneof<Lit<"u8">, Char<'u'>, Char<'U'>, Char<'L'>>;
using s_char = Oneof<escape_sequence, NotChar<'"', '\\', '\n'>>;
using s_char_sequence = Some<s_char>;
using string_literal = Seq<Opt<encoding_prefix>, Char<'"'>, Opt<s_char_sequence>, Char<'"'>>;

const char* match_string(const char* text, void* ctx) {
  using quote   = Char<'"'>;
  using escaped = Seq<Char<'\\'>, Char<>>;
  using item    = Oneof<splice, escaped, NotChar<'"'>>;
  using match   = Seq<Opt<encoding_prefix>, quote, Any<item>, quote>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
// Raw string literals - not in the spec, but useful

const char* match_raw_end(const char* text, const char* delim_a, const char* delim_b, void* ctx) {
  if (*text++ != ')') return nullptr;

  for(auto c = delim_a; c < delim_b; c++) {
    if (*text++ != *c) return nullptr;
  }

  if (*text++ != '"') return nullptr;

  return text;
}

const char* match_raw_string(const char* text, void* ctx) {
  using prefix = Oneof<Lit<"u8">, Char<'u'>, Char<'U'>, Char<'L'>>;
  using raw_prefix = Seq<Opt<prefix>, Lit<"R\"">>;

  text = raw_prefix::match(text, ctx);
  if (!text) return nullptr;

  auto delim_a = text;
  auto delim_b = Until<Char<'('>>::match(delim_a, ctx);
  if (!delim_b) return nullptr;

  for(text = delim_b + 1; text[0]; text++) {
    if (auto end = match_raw_end(text, delim_a, delim_b, ctx)) {
      return end;
    }
  }
  return nullptr;
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

const char* match_punct(const char* text, void* ctx) {
#if 0
  using trigraphs = Trigraphs<"...<<=>>=">;
  using digraphs  = Digraphs<"---=->!=*=/=&&&=##%=^=+++=<<<===>=>>|=||">;
  using unigraphs = Charset<"-,;:!?.()[]{}*/&#%^+<=>|~">;
  using match = Oneof<trigraphs, digraphs, unigraphs>;
  return match::match(text, ctx);
#endif

  using unigraphs = Charset<"-,;:!?.()[]{}*/&#%^+<=>|~">;
  return unigraphs::match(text, ctx);
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
// 6.4.8 Preprocessing numbers

/*
pp-number:
  digit
  . digit
  pp-number identifier_continue
  pp-number ’ digit
  pp-number ’ nondigit
  pp-number e sign
  pp-number E sign
  pp-number p sign
  pp-number P sign
  pp-number .
*/

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
// 6.10 Preprocessing directives

/*
preprocessing-file:
groupopt
group:
group-part
group group-part
group-part:
if-section
control-line
text-line
# non-directive
if-section:
if-group elif-groupsopt else-groupopt endif-line
if-group:
# if constant-expression new-line groupopt
# ifdef identifier new-line groupopt
# ifndef identifier new-line groupopt
elif-groups:
elif-group
elif-groups elif-group
elif-group:
# elif constant-expression new-line groupopt
# elifdef identifier new-line groupopt
# elifndef identifier new-line groupopt
else-group:
# else new-line groupopt
endif-line:
# endif new-line
control-line:
# include pp-tokens new-line
# embed pp-tokens new-line
# define identifier replacement-list new-line
# define identifier lparen identifier-listopt ) replacement-list new-line
# define identifier lparen ... ) replacement-list new-line
# define identifier lparen identifier-list , ... ) replacement-list new-line
# undef identifier new-line
# line pp-tokens new-line
# error pp-tokensopt new-line
# warning pp-tokensopt new-line
# pragma pp-tokensopt new-line
# new-line
text-line:
pp-tokensopt new-line
non-directive:
pp-tokens new-line
lparen:
a ( character not immediately preceded by white space
replacement-list:
pp-tokensopt
pp-tokens:
preprocessing-token
pp-tokens preprocessing-token
new-line:
the new-line character
identifier-list:
identifier
identifier-list , identifier
pp-parameter:
pp-parameter-name pp-parameter-clauseopt
pp-parameter-name:
pp-standard-parameter
pp-prefixed-parameter
pp-standard-parameter:
identifier
pp-prefixed-parameter:
identifier :: identifier
pp-parameter-clause:
( pp-balanced-token-sequenceopt )
pp-balanced-token-sequence:
pp-balanced-token
pp-balanced-token-sequence pp-balanced-token
pp-balanced-token:
( pp-balanced-token-sequenceopt )
[ pp-balanced-token-sequenceopt ]
{ pp-balanced-token-sequenceopt }
any pp-token other than a parenthesis, a bracket, or a brace
embed-parameter-sequence:
pp-parameter
embed-parameter-sequence pp-parameter
*/

const char* match_angle_path(const char* text, void* ctx) {
  // Quoted paths
  using quote     = Char<'"'>;
  using notquote  = NotChar<'"'>;
  using path1 = Seq<quote, Some<notquote>, quote>;

  // Angle-bracket paths
  using lbrack    = Char<'<'>;
  using rbrack    = Char<'>'>;
  using notbrack  = NotChar<'<', '>'>;
  using path2 = Seq<lbrack, Some<notbrack>, rbrack>;

  using match = Oneof<path1, path2>;
  return path2::match(text, ctx);
  //return match::match(text, ctx);
}

const char* match_preproc(const char* text, void* ctx) {

  // Raw strings in preprocs can make a preproc span multiple lines...?
  using item = Oneof<Ref<match_raw_string>, splice, NotChar<'\n'>>;

  using match = Seq<Char<'#'>, Any<item>, And<EOL>>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
