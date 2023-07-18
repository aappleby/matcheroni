// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

// #include "examples/c_parser/CContext.hpp"

#include "examples/c_lexer/CLexer.hpp"

#include "examples/SST.hpp"
#include "examples/c_lexer/CToken.hpp"
#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

template <typename M>
using ticked = Seq<Opt<Atom<'\''>>, M>;

// clang-format off
CToken    next_lexeme      (ContextBase& ctx, TextSpan s);
TextSpan  match_space      (ContextBase& ctx, TextSpan s);
TextSpan  match_newline    (ContextBase& ctx, TextSpan s);
TextSpan  match_string     (ContextBase& ctx, TextSpan s);
TextSpan  match_char       (ContextBase& ctx, TextSpan s);
TextSpan  match_keyword    (ContextBase& ctx, TextSpan s);
TextSpan  match_identifier (ContextBase& ctx, TextSpan s);
TextSpan  match_comment    (ContextBase& ctx, TextSpan s);
TextSpan  match_preproc    (ContextBase& ctx, TextSpan s);
TextSpan  match_float      (ContextBase& ctx, TextSpan s);
TextSpan  match_int        (ContextBase& ctx, TextSpan s);
TextSpan  match_punct      (ContextBase& ctx, TextSpan s);
TextSpan  match_splice     (ContextBase& ctx, TextSpan s);
TextSpan  match_formfeed   (ContextBase& ctx, TextSpan s);
TextSpan  match_eof        (ContextBase& ctx, TextSpan s);
// clang-format on

//------------------------------------------------------------------------------

CLexer::CLexer() { tokens.reserve(65536); }

void CLexer::reset() { tokens.clear(); }

//------------------------------------------------------------------------------

bool CLexer::lex(TextSpan text) {
  tokens.push_back(CToken(LEX_BOF, TextSpan(text.a, text.a)));


  ContextBase ctx;
  while (text) {
    // Don't pass a context here or we will slow way down doing rewinds
    auto token = next_lexeme(ctx, text);
    tokens.push_back(token);
    if (token.type == LEX_INVALID) {
      return false;
    }
    if (token.type == LEX_EOF) break;
    text.a = token.b;
  }

  return true;
}

//------------------------------------------------------------------------------

void CLexer::dump_lexemes() {
  for (auto& l : tokens) {
    l.dump();
    printf("\n");
  }
}

//------------------------------------------------------------------------------

CToken next_lexeme(ContextBase& ctx, TextSpan s) {
  if (auto end = match_space(ctx, s))   return CToken(LEX_SPACE, s - end);
  if (auto end = match_newline(ctx, s)) return CToken(LEX_NEWLINE, s - end);
  if (auto end = match_string(ctx, s))  return CToken(LEX_STRING, s - end);

  // Match char needs to come before match identifier because of its possible
  // L'_' prefix...
  if (auto end = match_char(ctx, s)) return CToken(LEX_CHAR, s - end);

  {
    auto end = match_identifier(ctx, s);
    if (end) {
      auto tok_span = s - end;
      if (SST<c_keywords>::match(tok_span.a, tok_span.b)) {
        return CToken(LEX_KEYWORD, tok_span);
      } else {
        return CToken(LEX_IDENTIFIER, tok_span);
      }
    }
  }

  if (auto end = match_comment(ctx, s))  return CToken(LEX_COMMENT, s - end);
  if (auto end = match_preproc(ctx, s))  return CToken(LEX_PREPROC, s - end);
  if (auto end = match_float(ctx, s))    return CToken(LEX_FLOAT, s - end);
  if (auto end = match_int(ctx, s))      return CToken(LEX_INT, s - end);
  if (auto end = match_punct(ctx, s))    return CToken(LEX_PUNCT, s - end);
  if (auto end = match_splice(ctx, s))   return CToken(LEX_SPLICE, s - end);
  if (auto end = match_formfeed(ctx, s)) return CToken(LEX_FORMFEED, s - end);
  if (auto end = match_eof(ctx, s))      return CToken(LEX_EOF, s - end);

  {
    if (auto end = match_string(ctx, s)) return CToken(LEX_STRING, s - end);
  }

  return CToken(LEX_INVALID, s.fail());
}

//------------------------------------------------------------------------------
// Misc helpers

TextSpan match_eof(ContextBase& ctx, TextSpan s) {
  if (s.is_empty()) return s;
  if (*s.a == 0) return TextSpan(s.a, s.a);
  return s.fail();
}

TextSpan match_formfeed(ContextBase& ctx, TextSpan s) {
  return Atom<'\f'>::match(ctx, s);
}

TextSpan match_space(ContextBase& ctx, TextSpan s) {
  using ws = Atom<' ', '\t'>;
  using pattern = Some<ws>;
  return pattern::match(ctx, s);
}

TextSpan match_newline(ContextBase& ctx, TextSpan s) {
  using pattern = Seq<Opt<Atom<'\r'>>, Atom<'\n'>>;
  auto end = pattern::match(ctx, s);
  return end;
}

//------------------------------------------------------------------------------
// 6.4.4.1 Integer constants

TextSpan match_int(ContextBase& ctx, TextSpan s) {
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

TextSpan match_universal_character_name(ContextBase& ctx, TextSpan s) {
  // clang-format off
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
  // clang-format on

  return universal_character_name::match(ctx, s);
}

//------------------------------------------------------------------------------
// Basic UTF8 support

TextSpan match_utf8(ContextBase& ctx, TextSpan s) {
  // clang-format off
  using utf8_ext       = Range<char(0x80), char(0xBF)>;
  //using utf8_onebyte   = Range<char(0x00), char(0x7F)>;
  using utf8_twobyte   = Seq<Range<char(0xC0), char(0xDF)>, utf8_ext>;
  using utf8_threebyte = Seq<Range<char(0xE0), char(0xEF)>, utf8_ext, utf8_ext>;
  using utf8_fourbyte  = Seq<Range<char(0xF0), char(0xF7)>, utf8_ext, utf8_ext, utf8_ext>;

  // matching 1-byte utf breaks things in match_identifier
  using utf8_char      = Oneof<utf8_twobyte, utf8_threebyte, utf8_fourbyte>;
  //using utf8_char      = Oneof<utf8_onebyte, utf8_twobyte, utf8_threebyte, utf8_fourbyte>;
  // clang-format on

  return utf8_char::match(ctx, s);
}

TextSpan match_utf8_bom(ContextBase& ctx, TextSpan s) {
  using utf8_bom = Seq<Atom<char(0xEF)>, Atom<char(0xBB)>, Atom<char(0xBF)>>;
  return utf8_bom::match(ctx, s);
}

//------------------------------------------------------------------------------
// 6.4.2 Identifiers - GCC allows dollar signs in identifiers?

TextSpan match_identifier(ContextBase& ctx, TextSpan s) {
  // clang-format off
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
  // clang-format on

  return identifier::match(ctx, s);
}

//------------------------------------------------------------------------------
// 6.4.4.2 Floating constants

TextSpan match_float(ContextBase& ctx, TextSpan s) {
  // clang-format off
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
  // clang-format on

  return floating_constant::match(ctx, s);
}

//------------------------------------------------------------------------------
// Escape sequences

TextSpan match_escape_sequence(ContextBase& ctx, TextSpan s) {
  // This is what's in the spec...
  // using simple_escape_sequence      = Seq<Atom<'\\'>,
  // Charset<"'\"?\\abfnrtv">>;

  // ...but GCC adds \e and \E, and '\(' '\{' '\[' '\%' are warnings but allowed

  // clang-format off
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
  // clang-format on

  return escape_sequence::match(ctx, s);
}

//------------------------------------------------------------------------------
// 6.4.4.4 Character constants

TextSpan match_char(ContextBase& ctx, TextSpan s) {
  // Multi-character character literals are allowed by spec, but their meaning
  // is implementation-defined...

  // clang-format off
  using c_char             = Oneof<Ref<match_escape_sequence>, NotAtom<'\'', '\\', '\n'>>;
  //using c_char_sequence    = Some<c_char>;

  using encoding_prefix    = Oneof<Lit<"u8">, Atom<'u', 'U', 'L'>>; // u8 must go first

  // The spec disallows empty character constants, but...
  //using character_constant = Seq< Opt<encoding_prefix>, Atom<'\''>, c_char_sequence, Atom<'\''> >;

  // ...in GCC they're only a warning.
  using character_constant = Seq< Opt<encoding_prefix>, Atom<'\''>, Any<c_char>, Atom<'\''> >;
  // clang-format on

  return character_constant::match(ctx, s);
}

//------------------------------------------------------------------------------
// 6.4.5 String literals

TextSpan match_cooked_string_literal(ContextBase& ctx, TextSpan s) {
  // Note, we add splices here since we're matching before preproccessing.

  // clang-format off
  using s_char          = Oneof<Ref<match_splice>, Ref<match_escape_sequence>, NotAtom<'"', '\\', '\n'>>;
  using s_char_sequence = Some<s_char>;
  using encoding_prefix = Oneof<Lit<"u8">, Atom<'u', 'U', 'L'>>; // u8 must go first
  using string_literal  = Seq<Opt<encoding_prefix>, Atom<'"'>, Opt<s_char_sequence>, Atom<'"'>>;
  // clang-format on

  return string_literal::match(ctx, s);
}

//----------------------------------------
// Raw string literals from the C++ spec

TextSpan match_raw_string_literal(ContextBase& ctx, TextSpan s) {
  // clang-format off
  using encoding_prefix    = Oneof<Lit<"u8">, Atom<'u', 'U', 'L'>>; // u8 must go first

  // We ignore backslash in d_char for similar splice-related reasons
  //using d_char          = NotAtom<' ', '(', ')', '\\', '\t', '\v', '\f', '\n'>;
  using d_char          = NotAtom<' ', '(', ')', '\t', '\v', '\f', '\n'>;
  using d_char_sequence = Some<d_char>;
  using backref_type    = Opt<d_char_sequence>;

  using r_terminator =
  Seq<
    Atom<')'>,
    MatchBackref<"raw_delim", char, backref_type>,
    Atom<'"'>
  >;

  using r_char          = Seq<Not<r_terminator>, AnyAtom>;
  using r_char_sequence = Some<r_char>;

  using raw_string_literal =
  Seq<
    Opt<encoding_prefix>,
    Atom<'R'>,
    Atom<'"'>,
    StoreBackref<"raw_delim", char, backref_type>,
    Atom<'('>,
    Opt<r_char_sequence>,
    Atom<')'>,
    MatchBackref<"raw_delim", char, backref_type>,
    Atom<'"'>
  >;
  // clang-format on

  return raw_string_literal::match(ctx, s);
}

//----------------------------------------

TextSpan match_string(ContextBase& ctx, TextSpan s) {
  // clang-format off
  using any_string = Oneof<
    Ref<match_cooked_string_literal>,
    Ref<match_raw_string_literal>
  >;
  // clang-format on

  return any_string::match(ctx, s);
}

//------------------------------------------------------------------------------
// 6.4.6 Punctuators

TextSpan match_punct(ContextBase& ctx, TextSpan s) {
  // We're just gonna match these one punct at a time
  using punctuator = Charset<"-,;:!?.()[]{}*/&#%^+<=>|~">;
  return punctuator::match(ctx, s);
}

// Yeaaaah, not gonna try to support trigraphs, they're obsolete and have been
// removed from the latest C spec. Also we have to declare them funny to get
// them through the preprocessor...
// using trigraphs = Trigraphs<R"(??=)" R"(??()" R"(??/)" R"(??))" R"(??')"
// R"(??<)" R"(??!)" R"(??>)" R"(??-)">;

//------------------------------------------------------------------------------
// 6.4.9 Comments

TextSpan match_oneline_comment(ContextBase& ctx, TextSpan s) {
  // Single-line comments
  using slc = Seq<Lit<"//">, Until<EOL>>;
  return slc::match(ctx, s);
}

TextSpan match_multiline_comment(ContextBase& ctx, TextSpan s) {
  // Multi-line non-nested comments
  using mlc_ldelim = Lit<"/*">;
  using mlc_rdelim = Lit<"*/">;
  using mlc = Seq<mlc_ldelim, Until<mlc_rdelim>, mlc_rdelim>;
  return mlc::match(ctx, s);
}

TextSpan match_comment(ContextBase& ctx, TextSpan s) {
  // clang-format off
  using comment =
  Oneof<
    Ref<match_oneline_comment>,
    Ref<match_multiline_comment>
  >;
  // clang-format on

  return comment::match(ctx, s);
}

//------------------------------------------------------------------------------
// 5.1.1.2 : Lines ending in a backslash and a newline get spliced together
// with the following line.

TextSpan match_splice(ContextBase& ctx, TextSpan s) {

  // According to GCC it's only a warning to have whitespace between the
  // backslash and the newline... and apparently \r\n is ok too?

  // clang-format off
  using splice = Seq<
    Atom<'\\'>,
    Any<Atom<' ','\t'>>,
    Opt<Atom<'\r'>>,
    Any<Atom<' ','\t'>>,
    Atom<'\n'>
  >;
  // clang-format on

  return splice::match(ctx, s);
}

//------------------------------------------------------------------------------

TextSpan match_preproc(ContextBase& ctx, TextSpan s) {
  // clang-format off
  using pattern = Seq<
    Atom<'#'>,
    Any<
      Ref<match_splice>,
      NotAtom<'\n'>
    >
  >;
  // clang-format on
  return pattern::match(ctx, s);
}

//------------------------------------------------------------------------------
