#include "c_lexer.h"
#include "Matcheroni.h"
#include <stdint.h>
#include <stdio.h>

using namespace matcheroni;

extern const char* current_filename;

//------------------------------------------------------------------------------
// Misc helpers

const char* match_space(const char* a, const char* b, void* ctx) {
  using ws = Atom<' ','\t'>;
  using match = Some<ws>;
  return match::match(a, b, ctx);
}

const char* match_newline(const char* a, const char* b, void* ctx) {
  using match = Seq<Opt<Atom<'\r'>>, Atom<'\n'>>;
  return match::match(a, b, ctx);
}

const char* match_str(const char* text, const char* lit) {
  while(1) {
    if (*text == 0 && *lit != 0) return nullptr;
    if (*text != 0 && *lit == 0) return text;
    if (*text == 0 && *lit == 0) return text;
    if (*text != *lit) return nullptr;
    text++;
    lit++;
  }

  return nullptr;
}

const char* match_str(const char* a, const char* b, const char* lit) {
  while(1) {
    if (*a == 0 && *lit != 0) return nullptr;
    if (*a != 0 && *lit == 0) return a;
    if (*a == 0 && *lit == 0) return a;
    if (*a != *lit) return nullptr;
    if (a == b) return nullptr;
    a++;
    lit++;
  }

  return nullptr;
}

template<typename M>
using ticked = Seq<Opt<Atom<'\''>>, M>;

//------------------------------------------------------------------------------
// Basic UTF8 support

const char* match_utf8(const char* a, const char* b, void* ctx) {
  using utf8_ext       = Range<char(0x80), char(0xBF)>;
  using utf8_onebyte   = Range<char(0x00), char(0x7F)>;
  using utf8_twobyte   = Seq<Range<char(0xC0), char(0xDF)>, utf8_ext>;
  using utf8_threebyte = Seq<Range<char(0xE0), char(0xEF)>, utf8_ext, utf8_ext>;
  using utf8_fourbyte  = Seq<Range<char(0xF0), char(0xF7)>, utf8_ext, utf8_ext, utf8_ext>;
  using utf8_char      = Oneof<utf8_onebyte, utf8_twobyte, utf8_threebyte, utf8_fourbyte>;

  return utf8_char::match(a, b, ctx);
}

const char* match_utf8_bom(const char* a, const char* b, void* ctx) {
  using utf8_bom = Seq<Atom<char(0xEF)>, Atom<char(0xBB)>, Atom<char(0xBF)>>;
  return utf8_bom::match(a, b, ctx);
}

// Yeaaaah, not gonna try to support trigraphs, they're obsolete and have been
// removed from the latest C spec. Also we have to declare them funny to get them through the preprocessor...
//using trigraphs = Trigraphs<R"(??=)" R"(??()" R"(??/)" R"(??))" R"(??')" R"(??<)" R"(??!)" R"(??>)" R"(??-)">;

//------------------------------------------------------------------------------
// 5.1.1.2 : Lines ending in a backslash and a newline get spliced together
// with the following line.

const char* match_splice(const char* a, const char* b, void* ctx) {
  // According to GCC it's only a warning to have whitespace between the
  // backslash and the newline... and apparently \r\n is ok too?

  using space = Atom<' ','\t'>;
  using splice = Seq<
    Atom<'\\'>,
    Any<space>,
    Opt<Atom<'\r'>>,
    Any<space>,
    Atom<'\n'>
  >;

  return splice::match(a, b, ctx);
}

//------------------------------------------------------------------------------
// 6.4.1 Keywords

// FIXME turn into "using keywords = Lits<c_keywords>;"

// Probably too many strings to pass as template parameters...
const char* c_keywords[] = {
  "alignas", "alignof", "auto", "bool", "break", "case", "char", "const",
  "constexpr", "continue", "default", "do", "double", "else", "enum",
  "extern", "false", "float", "for", "goto", "if", "inline", "int", "long",
  "nullptr", "register", "restrict", "return", "short", "signed", "sizeof",
  "static", "static_assert", "struct", "switch", "thread_local", "true",
  "typedef", "typeof", "typeof_unqual", "union", "unsigned", "void",
  "volatile", "while", "", "_Atomic", "_BitInt", "_Complex", "_Decimal128",
  "_Decimal32", "_Decimal64", "_Generic", "_Imaginary", "_Noreturn"
};
const int keyword_count = sizeof(c_keywords) / sizeof(c_keywords[0]);

const char* match_keyword(const char* text, void* ctx) {
  for (int i = 0; i < keyword_count; i++) {
    if (auto t = match_str(text, c_keywords[i])) {
      return t;
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
// 6.4.4.1 Integer constants

const char* match_int(const char* a, const char* b, void* ctx) {
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

  return integer_constant::match(a, b, ctx);
}

//------------------------------------------------------------------------------
// 6.4.3 Universal character names

const char* match_universal_character_name(const char* a, const char* b, void* ctx) {
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

  return universal_character_name::match(a, b, ctx);
}

//------------------------------------------------------------------------------
// 6.4.2 Identifiers - GCC allows dollar signs in identifiers?

const char* match_identifier(const char* a, const char* b, void* ctx) {
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

  return identifier::match(a, b, ctx);
}

//------------------------------------------------------------------------------
// 6.4.4.2 Floating constants

const char* match_float(const char* a, const char* b, void* ctx) {
  using floating_suffix = Oneof<
    Atom<'f'>, Atom<'l'>, Atom<'F'>, Atom<'L'>,
    Lit<"df">, Lit<"dd">, Lit<"dl">,
    Lit<"DF">, Lit<"DD">, Lit<"DL">
  >;

  using digit = Range<'0', '9'>;
  using ticked_digit   = Seq<Opt<Atom<'\''>>, digit>;
  using digit_sequence = Seq<digit, Any<ticked_digit>>;

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

  using decimal_floating_constant = Oneof<
    Seq< fractional_constant, Opt<exponent_part>, Opt<floating_suffix> >,
    Seq< digit_sequence, exponent_part, Opt<floating_suffix> >
  >;

  using hexadecimal_prefix         = Oneof<Lit<"0x">, Lit<"0X">>;

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

  return floating_constant::match(a, b, ctx);
}

//------------------------------------------------------------------------------
// 6.4.4.3 Enumeration constants

using enumeration_constant = Ref<match_identifier>;

//------------------------------------------------------------------------------
// Escape sequences

const char* match_escape_sequence(const char* a, const char* b, void* ctx) {
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

  return escape_sequence::match(a, b, ctx);
}

//------------------------------------------------------------------------------
// 6.4.4.4 Character constants

const char* match_character_constant(const char* a, const char* b, void* ctx) {
  // Multi-character character literals are allowed by spec, but their meaning is
  // implementation-defined...

  using c_char             = Oneof<Ref<match_escape_sequence>, NotAtom<'\'', '\\', '\n'>>;
  using c_char_sequence    = Some<c_char>;

  using encoding_prefix    = Oneof<Lit<"u8">, Atom<'u', 'U', 'L'>>; // u8 must go first

  // The spec disallows empty character constants, but...
  //using character_constant = Seq< Opt<encoding_prefix>, Atom<'\''>, c_char_sequence, Atom<'\''> >;

  // ...in GCC they're only a warning.
  using character_constant = Seq< Opt<encoding_prefix>, Atom<'\''>, Any<c_char>, Atom<'\''> >;

  return character_constant::match(a, b, ctx);
}

//------------------------------------------------------------------------------
// 6.4.4.5 Predefined constants

using predefined_constant = Oneof<Lit<"false">, Lit<"true">, Lit<"nullptr">>;

//------------------------------------------------------------------------------
// 6.4.5 String literals

const char* match_string_literal(const char* a, const char* b, void* ctx) {
  // Note, we add splices here since we're matching before preproccessing.
  using s_char          = Oneof<Ref<match_splice>, Ref<match_escape_sequence>, NotAtom<'"', '\\', '\n'>>;
  using s_char_sequence = Some<s_char>;
  using encoding_prefix    = Oneof<Lit<"u8">, Atom<'u', 'U', 'L'>>; // u8 must go first
  using string_literal  = Seq<Opt<encoding_prefix>, Atom<'"'>, Opt<s_char_sequence>, Atom<'"'>>;
  return string_literal::match(a, b, ctx);
}

//------------------------------------------------------------------------------
// Raw string literals from the C++ spec

const char* match_raw_string_literal(const char* a, const char* b, void* ctx) {

  using encoding_prefix    = Oneof<Lit<"u8">, Atom<'u', 'U', 'L'>>; // u8 must go first

  // We ignore backslash in d_char for similar splice-related reasons
  //using d_char          = NotAtom<' ', '(', ')', '\\', '\t', '\v', '\f', '\n'>;
  using d_char          = NotAtom<' ', '(', ')', '\t', '\v', '\f', '\n'>;
  using d_char_sequence = Some<d_char>;

  using r_terminator    = Seq<Atom<')'>, MatchBackref<char>, Atom<'"'>>;
  using r_char          = Seq<Not<r_terminator>, AnyAtom<char>>;
  using r_char_sequence = Some<r_char>;

  using raw_string_literal = Seq<
    Opt<encoding_prefix>,
    Atom<'R'>,
    Atom<'"'>,
    StoreBackref<Opt<d_char_sequence>>,
    Atom<'('>,
    Opt<r_char_sequence>,
    Atom<')'>,
    MatchBackref<char>,
    Atom<'"'>
  >;

  // Raw strings require matching backreferences, so we squish that into the
  // grammar by passing the backreference in the context pointer.
  Backref<char> backref;
  return raw_string_literal::match(a, b, &backref);
}

//------------------------------------------------------------------------------
// 6.4.6 Punctuators

const char* match_punct(const char* a, const char* b, void* ctx) {
  // We're just gonna match these one punct at a time
  using punctuator = Charset<"-,;:!?.()[]{}*/&#%^+<=>|~">;
  return punctuator::match(a, b, ctx);

#if 0
  using tripunct  = Trigraphs<"...<<=>>=">;
  using dipunct   = Digraphs<"---=->!=*=/=&&&=##%=^=+++=<<<===>=>>|=||">;
  using unipunct  = Charset<"-,;:!?.()[]{}*/&#%^+<=>|~">;
  using all_punct = Oneof<tripunct, dipunct, unipunct>;
  return all_punct::match(text, ctx);
#endif
}

//------------------------------------------------------------------------------
// 6.4.7 Header names

const char* match_header_name(const char* a, const char* b, void* ctx) {
  using q_char          = NotAtom<'\n', '"'>;
  using q_char_sequence = Some<q_char>;
  using h_char          = NotAtom<'\n', '>'>;
  using h_char_sequence = Some<h_char>;
  using header_name = Oneof<
    Seq< Atom<'<'>, h_char_sequence, Atom<'>'> >,
    Seq< Atom<'"'>, q_char_sequence, Atom<'"'> >
  >;

  return header_name::match(a, b, ctx);
}

//------------------------------------------------------------------------------
// 6.4.9 Comments

const char* match_oneline_comment(const char* a, const char* b, void* ctx) {
  // Single-line comments
  using slc = Seq<Lit<"//">, Until<EOL>>;
  return slc::match(a, b, ctx);
}

const char* match_multiline_comment(const char* a, const char* b, void* ctx) {
  // Multi-line non-nested comments
  using mlc_ldelim = Lit<"/*">;
  using mlc_rdelim = Lit<"*/">;
  using mlc  = Seq<mlc_ldelim, Until<mlc_rdelim>, mlc_rdelim>;
  return mlc::match(a, b, ctx);
}

#if 0
// Multi-line nested comments (not actually in C, just here for reference)

// FIXME make sure this matches the multiply-recursive version
const char* match_nested_comment(const char* text, void* ctx) {
  using ldelim = Lit<"/*">;
  using rdelim = Lit<"*/">;
  using item   = Oneof<Ref<match_nested_comment>, AnyAtom<char>>;
  using match  = Seq<ldelim, Any<item>, rdelim>;
  return match::match(text, ctx);
}

const char* match_nested_comment_body(const char* text, void* ctx);

const char* match_nested_comment(const char* text, void* ctx) {
  using ldelim = Lit<"/*">;
  using rdelim = Lit<"*/">;
  using item   = Seq<Not<ldelim>, Not<rdelim>, AnyAtom<char>>;
  using body   = Ref<match_nested_comment_body>;
  using match  = Seq<ldelim, body, rdelim>;
  return match::match(text, ctx);
}

const char* match_nested_comment_body(const char* text, void* ctx) {
  using ldelim = Lit<"/*">;
  using rdelim = Lit<"*/">;
  using item   = Seq<Not<ldelim>, Not<rdelim>, AnyAtom<char>>;
  using nested = Ref<match_nested_comment>;
  using match  = Any<Oneof<nested,item>>;
  return match::match(text, ctx);
}
#endif

//------------------------------------------------------------------------------
// We can't really handle preprocessing stuff so we're just matching the
// initial keyword.

const char* match_preproc(const char* a, const char* b, void* ctx) {
  if (*a != '#') return nullptr;
  a++;

  const char* preprocs[16] = {
    "define", "elif", "elifdef", "elifndef", "else", "embed", "endif", "error",
    "if", "ifdef", "ifndef", "include", "line", "pragma", "undef", "warning"
  };

  for (int i = 0; i < 16; i++) {
    if (auto t = match_str(a, b, preprocs[i])) {
      return t;
    }
  }

  return nullptr;
}

//------------------------------------------------------------------------------
