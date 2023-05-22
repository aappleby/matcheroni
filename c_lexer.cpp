#include "c_lexer.h"
#include "Matcheroni.h"
#include <stdint.h>
#include <stdio.h>

using namespace matcheroni;

extern const char* current_filename;

//------------------------------------------------------------------------------
// Misc

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
  auto end = match::match(text, ctx);

#if 1
  if (end) {
    printf("\n");
    printf("%s\n", current_filename);
    printf("{##########################{%.20s}##########################}\n", text);
    for (auto c = text; c < end; c++) {
      printf("{%c} 0x%02x ", *c, (unsigned char)(*c));
    }
    putc('\n', stdout);
    printf("\n");
  }
#endif

  return end;
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

using optional_tick = Opt<Char<'\''>>;

using digit                = Range<'0', '9'>;
using nonzero_digit        = Range<'1', '9'>;
using decimal_ticked_digit = Seq<optional_tick, digit>;
using decimal_constant     = Seq<nonzero_digit, Any<decimal_ticked_digit>>;

using hexadecimal_prefix         = Oneof<Lit<"0x">, Lit<"0X">>;
using hexadecimal_digit          = Oneof<digit, Range<'a', 'f'>, Range<'A', 'F'>>;
using hexadecimal_ticked_digit   = Seq<optional_tick, hexadecimal_digit>;
using hexadecimal_digit_sequence = Seq<hexadecimal_digit, Any<hexadecimal_ticked_digit>>;
using hexadecimal_constant       = Seq<hexadecimal_prefix, hexadecimal_digit_sequence>;

using binary_prefix         = Oneof<Lit<"0b">, Lit<"0B">>;
using binary_digit          = Char<'0','1'>;
using binary_ticked_digit   = Seq<optional_tick, binary_digit>;
using binary_digit_sequence = Seq<binary_digit, Any<binary_ticked_digit>>;
using binary_constant       = Seq<binary_prefix, binary_digit_sequence>;

using octal_digit        = Range<'0', '7'>;
using octal_ticked_digit = Seq<optional_tick, octal_digit>;
using octal_constant     = Seq<Char<'0'>, Any<octal_ticked_digit>>;

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

using hex_quad = Rep<4, hexadecimal_digit>;

using universal_character_name = Oneof<
  Seq< Lit<"\\u">, hex_quad >,
  Seq< Lit<"\\U">, hex_quad, hex_quad >,
  Seq< Lit<"\\u{">, Some<hexadecimal_digit>, Lit<"}">>
>;

//------------------------------------------------------------------------------
// 6.4.2 Identifiers - GCC allows dollar signs in identifiers?

using nondigit   = Oneof<Range<'a', 'z'>, Range<'A', 'Z'>, Char<'_'>, Char<'$'>, universal_character_name>;
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


using simple_escape_sequence      = Seq<Char<'\\'>, Charset<"'\"?\\abfnrtv">>;

using octal_escape_sequence = Oneof<
  Seq<Char<'\\'>, octal_digit, Opt<octal_digit>, Opt<octal_digit>>,
  Seq<Lit<'\\o{'>, Some<octal_digit>, Lit<"}">>,
>;

using hexadecimal_escape_sequence = Oneof<
  Seq<Lit<"\\x">, Some<hexadecimal_digit>>,
  Seq<Lit<"\\x{">, Some<hexadecimal_digit>, Lit<"}">>
>;

using n_char = NotChar<'}', '\n'>;
using named_escape_sequence = Seq<Lit<"\\N{">, Some<n_char>, Lit<"}">>;

using escape_sequence = Oneof<
  simple_escape_sequence,
  octal_escape_sequence,
  hexadecimal_escape_sequence,
  universal_character_name,
  named_escape_sequence
>;

const char* match_escape_sequence(const char* text, void* ctx) {
  return escape_sequence::match(text, ctx);
}

// Multi-character character literals are allowed by spec, but their meaning is
// implementation-defined...

using encoding_prefix    = Oneof<Lit<"u8">, Char<'u', 'U', 'L'>>; // u8 must go first
using c_char             = Oneof<escape_sequence, NotChar<'\'', '\\', '\n'>>;
using c_char_sequence    = Some<c_char>;
using character_constant = Seq< Opt<encoding_prefix>, Char<'\''>, c_char_sequence, Char<'\''> >;

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
  using item = Oneof<Ref<match_raw_string_literal>, splice, NotChar<'\n'>>;

  using match = Seq<Char<'#'>, Any<item>, And<EOL>>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
