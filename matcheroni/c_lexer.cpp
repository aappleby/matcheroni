#include "c_lexer.h"

#include "Lexemes.h"
#include "Matcheroni.h"

#include <stdint.h>
#include <stdio.h>
#include <array>
#include <cstdint>
#include <map>

#ifdef MATCHERONI_USE_NAMESPACE
using namespace matcheroni;
#endif

template<typename M>
using ticked = Seq<Opt<Atom<'\''>>, M>;

const char* match_never      (const char* a, const char* b);
const char* match_space      (const char* a, const char* b);
const char* match_newline    (const char* a, const char* b);
const char* match_string     (const char* a, const char* b);
const char* match_identifier (const char* a, const char* b);
const char* match_comment    (const char* a, const char* b);
const char* match_preproc    (const char* a, const char* b);
const char* match_float      (const char* a, const char* b);
const char* match_int        (const char* a, const char* b);
const char* match_punct      (const char* a, const char* b);
const char* match_char       (const char* a, const char* b);
const char* match_splice     (const char* a, const char* b);
const char* match_formfeed   (const char* a, const char* b);
const char* match_eof        (const char* a, const char* b);

//------------------------------------------------------------------------------

Lexeme next_lexeme(const char* cursor, const char* text_end) {
  struct LexemeTableEntry {
    matcher_function<const char> match  = nullptr;
    LexemeType    lexeme = LEX_INVALID;
  };

  static int blep[] = {
    [LEX_INVALID] = 1
  };

  /*
  // I think I sorted these in probability order...
  static LexemeTableEntry matcher_table[] = {
    //{ match_never,      LEX_INVALID,    },
    { match_space,      LEX_SPACE,      },
    { match_newline,    LEX_NEWLINE,    },
    { match_string,     LEX_STRING,     }, // must be before identifier because R"()"
    { match_identifier, LEX_IDENTIFIER, },
    { match_comment,    LEX_COMMENT,    }, // must be before punct
    { match_preproc,    LEX_PREPROC,    }, // must be before punct
    { match_float,      LEX_FLOAT,      }, // must be before int
    { match_int,        LEX_INT,        }, // must be before punct
    { match_punct,      LEX_PUNCT,      },
    { match_char,       LEX_CHAR,       },
    { match_splice,     LEX_SPLICE,     },
    { match_formfeed,   LEX_FORMFEED,   },
    { match_eof,        LEX_EOF,        },
  };

  const int matcher_count = sizeof(matcher_table) / sizeof(matcher_table[0]);
  for (int i = 0; i < matcher_count; i++) {
    if (auto end = matcher_table[i].match(cursor, text_end)) {
      return Lexeme(matcher_table[i].lexeme, cursor, end);
    }
  }
  */

  if (auto end = match_space      (cursor, text_end)) return Lexeme(LEX_SPACE     , cursor, end);
  if (auto end = match_newline    (cursor, text_end)) return Lexeme(LEX_NEWLINE   , cursor, end);
  if (auto end = match_string     (cursor, text_end)) return Lexeme(LEX_STRING    , cursor, end);
  if (auto end = match_identifier (cursor, text_end)) return Lexeme(LEX_IDENTIFIER, cursor, end);
  if (auto end = match_comment    (cursor, text_end)) return Lexeme(LEX_COMMENT   , cursor, end);
  if (auto end = match_preproc    (cursor, text_end)) return Lexeme(LEX_PREPROC   , cursor, end);
  if (auto end = match_float      (cursor, text_end)) return Lexeme(LEX_FLOAT     , cursor, end);
  if (auto end = match_int        (cursor, text_end)) return Lexeme(LEX_INT       , cursor, end);
  if (auto end = match_punct      (cursor, text_end)) return Lexeme(LEX_PUNCT     , cursor, end);
  if (auto end = match_char       (cursor, text_end)) return Lexeme(LEX_CHAR      , cursor, end);
  if (auto end = match_splice     (cursor, text_end)) return Lexeme(LEX_SPLICE    , cursor, end);
  if (auto end = match_formfeed   (cursor, text_end)) return Lexeme(LEX_FORMFEED  , cursor, end);
  if (auto end = match_eof        (cursor, text_end)) return Lexeme(LEX_EOF       , cursor, end);


  return Lexeme(LEX_INVALID, nullptr, nullptr);
}

//------------------------------------------------------------------------------
// Misc helpers

const char* match_never(const char* a, const char* b) {
  return nullptr;
}

const char* match_eof(const char* a, const char* b) {
  if (a == b) return a;
  return nullptr;
}

const char* match_formfeed(const char* a, const char* b) {
  return Atom<'\f'>::match(a, b);
}

const char* match_space(const char* a, const char* b) {
  using ws = Atom<' ','\t'>;
  using match = Some<ws>;
  return match::match(a, b);
}

const char* match_newline(const char* a, const char* b) {
  using match = Seq<Opt<Atom<'\r'>>, Atom<'\n'>>;
  return match::match(a, b);
}

//------------------------------------------------------------------------------
// 6.4.4.1 Integer constants

const char* match_int(const char* a, const char* b) {
  using digit                = Range<'0', '9'>;
  using nonzero_digit        = Range<'1', '9'>;

  using decimal_constant     = Seq<nonzero_digit, Any<ticked<digit>>>;

  using hexadecimal_prefix         = Oneof<Lit<"0x">, Lit<"0X">>;
  using hexadecimal_digit          = Oneof<Range<'0', '9'>, Range<'a', 'f'>, Range<'A', 'F'>>;
  using hexadecimal_digit_sequence = Seq<hexadecimal_digit, Any<ticked<hexadecimal_digit>>>;
  using hexadecimal_constant       = Seq<hexadecimal_prefix, hexadecimal_digit_sequence>;

  using binary_prefix         = Oneof<Lit<"0b">, Lit<"0B">>;
  using binary_digit          = Atom<'0','1'>;
  using binary_digit_sequence = Seq<binary_digit, Any<ticked<binary_digit>>>;
  using binary_constant       = Seq<binary_prefix, binary_digit_sequence>;

  using octal_digit        = Range<'0', '7'>;
  using octal_constant     = Seq<Atom<'0'>, Any<ticked<octal_digit>>>;

  using unsigned_suffix        = Atom<'u', 'U'>;
  using long_suffix            = Atom<'l', 'L'>;
  using long_long_suffix       = Oneof<Lit<"ll">, Lit<"LL">>;
  using bit_precise_int_suffix = Oneof<Lit<"wb">, Lit<"WB">>;

  // This is a little odd because we have to match in longest-suffix-first order
  // to ensure we capture the entire suffix
  using integer_suffix = Oneof<
    Seq<unsigned_suffix,  long_long_suffix>,
    Seq<unsigned_suffix,  long_suffix>,
    Seq<unsigned_suffix,  bit_precise_int_suffix>,
    Seq<unsigned_suffix>,

    Seq<long_long_suffix,       Opt<unsigned_suffix>>,
    Seq<long_suffix,            Opt<unsigned_suffix>>,
    Seq<bit_precise_int_suffix, Opt<unsigned_suffix>>
  >;

  // GCC allows i or j in addition to the normal suffixes for complex-ified types :/...
  using complex_suffix = Atom<'i', 'j'>;

  // Octal has to be _after_ bin/hex so we don't prematurely match the prefix
  using integer_constant =
  Seq<
    Oneof<
      decimal_constant,
      hexadecimal_constant,
      binary_constant,
      octal_constant
    >,
    Seq<
      Opt<complex_suffix>,
      Opt<integer_suffix>,
      Opt<complex_suffix>
    >
  >;

  return integer_constant::match(a, b);
}

//------------------------------------------------------------------------------
// 6.4.3 Universal character names

const char* match_universal_character_name(const char* a, const char* b) {
  using n_char = NotAtom<'}','\n'>;
  using n_char_sequence = Some<n_char>;
  using named_universal_character = Seq<Lit<"\\N{">, n_char_sequence, Lit<"}">>;

  using hexadecimal_digit = Oneof<Range<'0', '9'>, Range<'a', 'f'>, Range<'A', 'F'>>;
  using hex_quad = Rep<4, hexadecimal_digit>;

  using universal_character_name = Oneof<
    Seq< Lit<"\\u">, hex_quad >,
    Seq< Lit<"\\U">, hex_quad, hex_quad >,
    Seq< Lit<"\\u{">, Any<hexadecimal_digit>, Lit<"}">>,
    named_universal_character
  >;

  return universal_character_name::match(a, b);
}

//------------------------------------------------------------------------------
// Basic UTF8 support

const char* match_utf8(const char* a, const char* b) {
  using utf8_ext       = Range<char(0x80), char(0xBF)>;
  using utf8_onebyte   = Range<char(0x00), char(0x7F)>;
  using utf8_twobyte   = Seq<Range<char(0xC0), char(0xDF)>, utf8_ext>;
  using utf8_threebyte = Seq<Range<char(0xE0), char(0xEF)>, utf8_ext, utf8_ext>;
  using utf8_fourbyte  = Seq<Range<char(0xF0), char(0xF7)>, utf8_ext, utf8_ext, utf8_ext>;

  // matching 1-byte utf breaks things in match_identifier
  using utf8_char      = Oneof<utf8_twobyte, utf8_threebyte, utf8_fourbyte>;
  //using utf8_char      = Oneof<utf8_onebyte, utf8_twobyte, utf8_threebyte, utf8_fourbyte>;

  return utf8_char::match(a, b);
}

const char* match_utf8_bom(const char* a, const char* b) {
  using utf8_bom = Seq<Atom<char(0xEF)>, Atom<char(0xBB)>, Atom<char(0xBF)>>;
  return utf8_bom::match(a, b);
}

//------------------------------------------------------------------------------
// 6.4.2 Identifiers - GCC allows dollar signs in identifiers?

const char* match_identifier(const char* a, const char* b) {
  using digit = Range<'0', '9'>;

  // Not sure if this should be in here
  using latin1_ext = Range<char(128),char(255)>;

  using nondigit = Oneof<
    Range<'a', 'z'>,
    Range<'A', 'Z'>,
    Atom<'_'>,
    Atom<'$'>,
    Ref<match_universal_character_name>,
    latin1_ext, // One of the GCC test files requires this
    Ref<match_utf8>  // Lots of GCC test files for unicode
  >;

  using identifier = Seq<nondigit, Any<Oneof<digit, nondigit>>>;

  return identifier::match(a, b);
}

//------------------------------------------------------------------------------
// 6.4.4.2 Floating constants

const char* match_float(const char* a, const char* b) {
  using floating_suffix = Oneof<
    Atom<'f'>, Atom<'l'>, Atom<'F'>, Atom<'L'>,
    Lit<"df">, Lit<"dd">, Lit<"dl">,
    Lit<"DF">, Lit<"DD">, Lit<"DL">
  >;

  using digit = Range<'0', '9'>;

  using digit_sequence = Seq<digit, Any<ticked<digit>>>;

  using fractional_constant = Oneof<
    Seq<Opt<digit_sequence>, Atom<'.'>, digit_sequence>,
    Seq<digit_sequence, Atom<'.'>>
  >;

  using sign = Atom<'+','-'>;

  using hexadecimal_digit          = Oneof<Range<'0', '9'>, Range<'a', 'f'>, Range<'A', 'F'>>;
  using hexadecimal_digit_sequence = Seq<hexadecimal_digit, Any<ticked<hexadecimal_digit>>>;
  using hexadecimal_fractional_constant = Oneof<
    Seq<Opt<hexadecimal_digit_sequence>, Atom<'.'>, hexadecimal_digit_sequence>,
    Seq<hexadecimal_digit_sequence, Atom<'.'>>
  >;

  using exponent_part        = Seq<Atom<'e', 'E'>, Opt<sign>, digit_sequence>;
  using binary_exponent_part = Seq<Atom<'p', 'P'>, Opt<sign>, digit_sequence>;

  // GCC allows i or j in addition to the normal suffixes for complex-ified types :/...
  using complex_suffix = Atom<'i', 'j'>;

  using decimal_floating_constant = Oneof<
    Seq< Opt<sign>, fractional_constant, Opt<exponent_part>, Opt<complex_suffix>, Opt<floating_suffix>, Opt<complex_suffix> >,
    Seq< Opt<sign>, digit_sequence, exponent_part,           Opt<complex_suffix>, Opt<floating_suffix>, Opt<complex_suffix> >
  >;

  using hexadecimal_prefix         = Oneof<Lit<"0x">, Lit<"0X">>;

  using hexadecimal_floating_constant = Seq<
    Opt<sign>,
    hexadecimal_prefix,
    Oneof<hexadecimal_fractional_constant, hexadecimal_digit_sequence>,
    binary_exponent_part,
    Opt<complex_suffix>,
    Opt<floating_suffix>,
    Opt<complex_suffix>
  >;

  using floating_constant = Oneof<
    decimal_floating_constant,
    hexadecimal_floating_constant
  >;

  return floating_constant::match(a, b);
}

//------------------------------------------------------------------------------
// Escape sequences

const char* match_escape_sequence(const char* a, const char* b) {
  // This is what's in the spec...
  //using simple_escape_sequence      = Seq<Atom<'\\'>, Charset<"'\"?\\abfnrtv">>;

  // ...but GCC adds \e and \E, and '\(' '\{' '\[' '\%' are warnings but allowed
  using simple_escape_sequence      = Seq<Atom<'\\'>, Charset<"'\"?\\abfnrtveE({[%">>;

  using octal_digit = Range<'0', '7'>;
  using octal_escape_sequence = Oneof<
    Seq<Atom<'\\'>, octal_digit, Opt<octal_digit>, Opt<octal_digit>>,
    Seq<Lit<"\\o{">, Any<octal_digit>, Lit<"}">>
  >;

  using hexadecimal_digit = Oneof<Range<'0', '9'>, Range<'a', 'f'>, Range<'A', 'F'>>;
  using hexadecimal_escape_sequence = Oneof<
    Seq<Lit<"\\x">, Some<hexadecimal_digit>>,
    Seq<Lit<"\\x{">, Any<hexadecimal_digit>, Lit<"}">>
  >;

  using escape_sequence = Oneof<
    simple_escape_sequence,
    octal_escape_sequence,
    hexadecimal_escape_sequence,
    Ref<match_universal_character_name>
  >;

  return escape_sequence::match(a, b);
}

//------------------------------------------------------------------------------
// 6.4.4.4 Character constants

const char* match_char(const char* a, const char* b) {
  // Multi-character character literals are allowed by spec, but their meaning is
  // implementation-defined...

  using c_char             = Oneof<Ref<match_escape_sequence>, NotAtom<'\'', '\\', '\n'>>;
  using c_char_sequence    = Some<c_char>;

  using encoding_prefix    = Oneof<Lit<"u8">, Atom<'u', 'U', 'L'>>; // u8 must go first

  // The spec disallows empty character constants, but...
  //using character_constant = Seq< Opt<encoding_prefix>, Atom<'\''>, c_char_sequence, Atom<'\''> >;

  // ...in GCC they're only a warning.
  using character_constant = Seq< Opt<encoding_prefix>, Atom<'\''>, Any<c_char>, Atom<'\''> >;

  return character_constant::match(a, b);
}

//------------------------------------------------------------------------------
// 6.4.5 String literals

const char* match_cooked_string_literal(const char* a, const char* b) {
  // Note, we add splices here since we're matching before preproccessing.
  using s_char          = Oneof<Ref<match_splice>, Ref<match_escape_sequence>, NotAtom<'"', '\\', '\n'>>;
  using s_char_sequence = Some<s_char>;
  using encoding_prefix    = Oneof<Lit<"u8">, Atom<'u', 'U', 'L'>>; // u8 must go first
  using string_literal  = Seq<Opt<encoding_prefix>, Atom<'"'>, Opt<s_char_sequence>, Atom<'"'>>;
  return string_literal::match(a, b);
}

//------------------------------------------------------------------------------
// Raw string literals from the C++ spec

const char* match_raw_string_literal(const char* a, const char* b) {

  using encoding_prefix    = Oneof<Lit<"u8">, Atom<'u', 'U', 'L'>>; // u8 must go first

  // We ignore backslash in d_char for similar splice-related reasons
  //using d_char          = NotAtom<' ', '(', ')', '\\', '\t', '\v', '\f', '\n'>;
  using d_char          = NotAtom<' ', '(', ')', '\t', '\v', '\f', '\n'>;
  using d_char_sequence = Some<d_char>;
  using backref_type    = Opt<d_char_sequence>;

  using r_terminator    = Seq<Atom<')'>, MatchBackref<backref_type>, Atom<'"'>>;
  using r_char          = Seq<Not<r_terminator>, AnyAtom>;
  using r_char_sequence = Some<r_char>;

  using raw_string_literal = Seq<
    Opt<encoding_prefix>,
    Atom<'R'>,
    Atom<'"'>,
    StoreBackref<backref_type>,
    Atom<'('>,
    Opt<r_char_sequence>,
    Atom<')'>,
    MatchBackref<backref_type>,
    Atom<'"'>
  >;

  return raw_string_literal::match(a, b);
}

const char* match_string(const char* a, const char* b) {
  using any_string = Oneof<
    Ref<match_cooked_string_literal>,
    Ref<match_raw_string_literal>
  >;

  return any_string::match(a, b);
}

//------------------------------------------------------------------------------
// 6.4.6 Punctuators

const char* match_punct(const char* a, const char* b) {
  // We're just gonna match these one punct at a time
  using punctuator = Charset<"-,;:!?.()[]{}*/&#%^+<=>|~">;
  return punctuator::match(a, b);
}

// Yeaaaah, not gonna try to support trigraphs, they're obsolete and have been
// removed from the latest C spec. Also we have to declare them funny to get them through the preprocessor...
//using trigraphs = Trigraphs<R"(??=)" R"(??()" R"(??/)" R"(??))" R"(??')" R"(??<)" R"(??!)" R"(??>)" R"(??-)">;

//------------------------------------------------------------------------------
// 6.4.9 Comments

const char* match_oneline_comment(const char* a, const char* b) {
  // Single-line comments
  using slc = Seq<Lit<"//">, Until<EOL>>;
  return slc::match(a, b);
}

const char* match_multiline_comment(const char* a, const char* b) {
  // Multi-line non-nested comments
  using mlc_ldelim = Lit<"/*">;
  using mlc_rdelim = Lit<"*/">;
  using mlc  = Seq<mlc_ldelim, Until<mlc_rdelim>, mlc_rdelim>;
  return mlc::match(a, b);
}

const char* match_comment(const char* a, const char* b) {
  using comment = Oneof<
    Ref<match_oneline_comment>,
    Ref<match_multiline_comment>
  >;

  return comment::match(a, b);
}

//------------------------------------------------------------------------------
// 5.1.1.2 : Lines ending in a backslash and a newline get spliced together
// with the following line.

const char* match_splice(const char* a, const char* b) {
  // According to GCC it's only a warning to have whitespace between the
  // backslash and the newline... and apparently \r\n is ok too?

  using splice = Seq<
    Atom<'\\'>,
    Any<Atom<' ','\t'>>,
    Opt<Atom<'\r'>>,
    Any<Atom<' ','\t'>>,
    Atom<'\n'>
  >;

  return splice::match(a, b);
}

//------------------------------------------------------------------------------

const char* match_preproc(const char* a, const char* b) {
  using pattern = Seq<
    Atom<'#'>,
    Any<Oneof<
      Ref<match_splice>,
      NotAtom<'\n'>
    >>
  >;
  return pattern::match(a, b);
}

//------------------------------------------------------------------------------
