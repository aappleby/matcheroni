// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

//#include "examples/c99_parser/C99Parser.hpp"

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

#include "examples/c99_parser/C99Lexer.hpp"
#include "examples/c99_parser/Lexeme.hpp"
#include "examples/c99_parser/SST.hpp"

using namespace matcheroni;

template<typename M>
using ticked = Seq<Opt<Atom<'\''>>, M>;

Lexeme next_lexeme      (void* ctx, cspan s);
cspan  match_never      (void* ctx, cspan s);
cspan  match_space      (void* ctx, cspan s);
cspan  match_newline    (void* ctx, cspan s);
cspan  match_string     (void* ctx, cspan s);
cspan  match_char       (void* ctx, cspan s);
cspan  match_keyword    (void* ctx, cspan s);
cspan  match_identifier (void* ctx, cspan s);
cspan  match_comment    (void* ctx, cspan s);
cspan  match_preproc    (void* ctx, cspan s);
cspan  match_float      (void* ctx, cspan s);
cspan  match_int        (void* ctx, cspan s);
cspan  match_punct      (void* ctx, cspan s);
cspan  match_splice     (void* ctx, cspan s);
cspan  match_formfeed   (void* ctx, cspan s);
cspan  match_eof        (void* ctx, cspan s);

//------------------------------------------------------------------------------

C99Lexer::C99Lexer() {
  lexemes.reserve(65536);
}

void C99Lexer::reset() {
  lexemes.clear();
}

//------------------------------------------------------------------------------

bool C99Lexer::lex(const std::string& text) {

  auto s = matcheroni::to_span(text);

  lexemes.push_back(Lexeme(LEX_BOF, cspan(nullptr, nullptr)));

  while (!s.is_empty()) {
    auto lex = next_lexeme(nullptr, s);
    lexemes.push_back(lex);

    if (lex.type == LEX_INVALID) {
      return false;
    }
    if (lex.type == LEX_EOF) {
      break;
    }

    s.a = lex.span.b;
  }


  return true;
}

//------------------------------------------------------------------------------

void C99Lexer::dump_lexemes() {
  for (auto& l : lexemes) {
    printf("{");
    l.dump_lexeme();
    printf("}");
    printf("\n");
  }
}

//------------------------------------------------------------------------------

//inline int atom_cmp(void* ctx, cspan* s, const char* b) {
//  return strcmp_span(s, b);
//}

Lexeme next_lexeme(void* ctx, cspan s) {

  if (auto end = match_space  (ctx, s)) return Lexeme(LEX_SPACE,   s - end);
  if (auto end = match_newline(ctx, s)) return Lexeme(LEX_NEWLINE, s - end);
  if (auto end = match_string (ctx, s)) return Lexeme(LEX_STRING,  s - end);

  // Match char needs to come before match identifier because of its possible L'_' prefix...
  if (auto end = match_char       (ctx, s)) return Lexeme(LEX_CHAR, s - end);

  {
    auto end = match_identifier(ctx, s);
    if (end) {
      if (SST<c99_keywords>::match(s.a, s.b)) {
        return Lexeme(LEX_KEYWORD   , s - end);
      }
      else {
        return Lexeme(LEX_IDENTIFIER, s - end);
      }
    }
  }

  if (auto end = match_comment(ctx, s))  return Lexeme(LEX_COMMENT,  s - end);
  if (auto end = match_preproc(ctx, s))  return Lexeme(LEX_PREPROC,  s - end);
  if (auto end = match_float(ctx, s))    return Lexeme(LEX_FLOAT,    s - end);
  if (auto end = match_int(ctx, s))      return Lexeme(LEX_INT,      s - end);
  if (auto end = match_punct(ctx, s))    return Lexeme(LEX_PUNCT,    s - end);
  if (auto end = match_splice(ctx, s))   return Lexeme(LEX_SPLICE,   s - end);
  if (auto end = match_formfeed(ctx, s)) return Lexeme(LEX_FORMFEED, s - end);
  if (auto end = match_eof(ctx, s))      return Lexeme(LEX_EOF,      s - end);

  return Lexeme(LEX_INVALID, s.fail());
}

//------------------------------------------------------------------------------
// Misc helpers

cspan match_never(void* ctx, cspan s) {
  return s.fail();
}

cspan match_eof(void* ctx, cspan s) {
  if (s.is_empty()) return s;
  return s.fail();
}

cspan match_formfeed(void* ctx, cspan s) {
  return Atom<'\f'>::match(ctx, s);
}

cspan match_space(void* ctx, cspan s) {
  using ws = Atom<' ','\t'>;
  using pattern = Some<ws>;
  return pattern::match(ctx, s);
}

cspan match_newline(void* ctx, cspan s) {
  using pattern = Seq<Opt<Atom<'\r'>>, Atom<'\n'>>;
  return pattern::match(ctx, s);
}

//------------------------------------------------------------------------------
// 6.4.4.1 Integer constants

cspan match_int(void* ctx, cspan s) {
  // clang-format off
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

  return integer_constant::match(ctx, s);
  // clang-format on
}

//------------------------------------------------------------------------------
// 6.4.3 Universal character names

cspan match_universal_character_name(void* ctx, cspan s) {
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

  return universal_character_name::match(ctx, s);
}

//------------------------------------------------------------------------------
// Basic UTF8 support

cspan match_utf8(void* ctx, cspan s) {
  using utf8_ext       = Range<char(0x80), char(0xBF)>;
  //using utf8_onebyte   = Range<char(0x00), char(0x7F)>;
  using utf8_twobyte   = Seq<Range<char(0xC0), char(0xDF)>, utf8_ext>;
  using utf8_threebyte = Seq<Range<char(0xE0), char(0xEF)>, utf8_ext, utf8_ext>;
  using utf8_fourbyte  = Seq<Range<char(0xF0), char(0xF7)>, utf8_ext, utf8_ext, utf8_ext>;

  // matching 1-byte utf breaks things in match_identifier
  using utf8_char      = Oneof<utf8_twobyte, utf8_threebyte, utf8_fourbyte>;
  //using utf8_char      = Oneof<utf8_onebyte, utf8_twobyte, utf8_threebyte, utf8_fourbyte>;

  return utf8_char::match(ctx, s);
}

cspan match_utf8_bom(void* ctx, cspan s) {
  using utf8_bom = Seq<Atom<char(0xEF)>, Atom<char(0xBB)>, Atom<char(0xBF)>>;
  return utf8_bom::match(ctx, s);
}

//------------------------------------------------------------------------------
// 6.4.2 Identifiers - GCC allows dollar signs in identifiers?

cspan match_identifier(void* ctx, cspan s) {
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

  using identifier = Seq<nondigit, Any<digit, nondigit>>;

  return identifier::match(ctx, s);
}

//------------------------------------------------------------------------------
// 6.4.4.2 Floating constants

cspan match_float(void* ctx, cspan s) {
  using floating_suffix = Oneof<
    Atom<'f'>, Atom<'l'>, Atom<'F'>, Atom<'L'>,
    // Decimal floats, GCC thing
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
    Seq< fractional_constant, Opt<exponent_part>, Opt<complex_suffix>, Opt<floating_suffix>, Opt<complex_suffix> >,
    Seq< digit_sequence, exponent_part,           Opt<complex_suffix>, Opt<floating_suffix>, Opt<complex_suffix> >
  >;

  using hexadecimal_prefix         = Oneof<Lit<"0x">, Lit<"0X">>;

  using hexadecimal_floating_constant = Seq<
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

  return floating_constant::match(ctx, s);
}

//------------------------------------------------------------------------------
// Escape sequences

cspan match_escape_sequence(void* ctx, cspan s) {
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

  return escape_sequence::match(ctx, s);
}

//------------------------------------------------------------------------------
// 6.4.4.4 Character constants

cspan match_char(void* ctx, cspan s) {
  // Multi-character character literals are allowed by spec, but their meaning is
  // implementation-defined...

  using c_char             = Oneof<Ref<match_escape_sequence>, NotAtom<'\'', '\\', '\n'>>;
  //using c_char_sequence    = Some<c_char>;

  using encoding_prefix    = Oneof<Lit<"u8">, Atom<'u', 'U', 'L'>>; // u8 must go first

  // The spec disallows empty character constants, but...
  //using character_constant = Seq< Opt<encoding_prefix>, Atom<'\''>, c_char_sequence, Atom<'\''> >;

  // ...in GCC they're only a warning.
  using character_constant = Seq< Opt<encoding_prefix>, Atom<'\''>, Any<c_char>, Atom<'\''> >;

  return character_constant::match(ctx, s);
}

//------------------------------------------------------------------------------
// 6.4.5 String literals

cspan match_cooked_string_literal(void* ctx, cspan s) {
  // Note, we add splices here since we're matching before preproccessing.
  using s_char          = Oneof<Ref<match_splice>, Ref<match_escape_sequence>, NotAtom<'"', '\\', '\n'>>;
  using s_char_sequence = Some<s_char>;
  using encoding_prefix    = Oneof<Lit<"u8">, Atom<'u', 'U', 'L'>>; // u8 must go first
  using string_literal  = Seq<Opt<encoding_prefix>, Atom<'"'>, Opt<s_char_sequence>, Atom<'"'>>;
  return string_literal::match(ctx, s);
}

//----------------------------------------
// Raw string literals from the C++ spec

cspan match_raw_string_literal(void* ctx, cspan s) {

  using encoding_prefix    = Oneof<Lit<"u8">, Atom<'u', 'U', 'L'>>; // u8 must go first

  // We ignore backslash in d_char for similar splice-related reasons
  //using d_char          = NotAtom<' ', '(', ')', '\\', '\t', '\v', '\f', '\n'>;
  using d_char          = NotAtom<' ', '(', ')', '\t', '\v', '\f', '\n'>;
  using d_char_sequence = Some<d_char>;
  using backref_type    = Opt<d_char_sequence>;

  using r_terminator    = Seq<Atom<')'>, MatchBackref<"raw_delim", const char, backref_type>, Atom<'"'>>;
  using r_char          = Seq<Not<r_terminator>, AnyAtom>;
  using r_char_sequence = Some<r_char>;

  using raw_string_literal = Seq<
    Opt<encoding_prefix>,
    Atom<'R'>,
    Atom<'"'>,
    StoreBackref<"raw_delim", const char, backref_type>,
    Atom<'('>,
    Opt<r_char_sequence>,
    Atom<')'>,
    MatchBackref<"raw_delim", const char, backref_type>,
    Atom<'"'>
  >;

  return raw_string_literal::match(ctx, s);
}

//----------------------------------------

cspan match_string(void* ctx, cspan s) {
  using any_string = Oneof<
    Ref<match_cooked_string_literal>,
    Ref<match_raw_string_literal>
  >;

  return any_string::match(ctx, s);
}

//------------------------------------------------------------------------------
// 6.4.6 Punctuators

cspan match_punct(void* ctx, cspan s) {
  // We're just gonna match these one punct at a time
  using punctuator = Charset<"-,;:!?.()[]{}*/&#%^+<=>|~">;
  return punctuator::match(ctx, s);
}

// Yeaaaah, not gonna try to support trigraphs, they're obsolete and have been
// removed from the latest C spec. Also we have to declare them funny to get them through the preprocessor...
//using trigraphs = Trigraphs<R"(??=)" R"(??()" R"(??/)" R"(??))" R"(??')" R"(??<)" R"(??!)" R"(??>)" R"(??-)">;

//------------------------------------------------------------------------------
// 6.4.9 Comments

cspan match_oneline_comment(void* ctx, cspan s) {
  // Single-line comments
  using slc = Seq<Lit<"//">, Until<EOL>>;
  return slc::match(ctx, s);
}

cspan match_multiline_comment(void* ctx, cspan s) {
  // Multi-line non-nested comments
  using mlc_ldelim = Lit<"/*">;
  using mlc_rdelim = Lit<"*/">;
  using mlc  = Seq<mlc_ldelim, Until<mlc_rdelim>, mlc_rdelim>;
  return mlc::match(ctx, s);
}

cspan match_comment(void* ctx, cspan s) {
  using comment = Oneof<
    Ref<match_oneline_comment>,
    Ref<match_multiline_comment>
  >;

  return comment::match(ctx, s);
}

//------------------------------------------------------------------------------
// 5.1.1.2 : Lines ending in a backslash and a newline get spliced together
// with the following line.

cspan match_splice(void* ctx, cspan s) {
  // According to GCC it's only a warning to have whitespace between the
  // backslash and the newline... and apparently \r\n is ok too?

  using splice = Seq<
    Atom<'\\'>,
    Any<Atom<' ','\t'>>,
    Opt<Atom<'\r'>>,
    Any<Atom<' ','\t'>>,
    Atom<'\n'>
  >;

  return splice::match(ctx, s);
}

//------------------------------------------------------------------------------

cspan match_preproc(void* ctx, cspan s) {
  using pattern = Seq<
    Atom<'#'>,
    Any<
      Ref<match_splice>,
      NotAtom<'\n'>
    >
  >;
  return pattern::match(ctx, s);
}

//------------------------------------------------------------------------------
