#include "examples.h"
#include "Matcheroni.h"
#include <stdint.h>
#include <stdio.h>

using namespace matcheroni;

//------------------------------------------------------------------------------
// Lines ending in a backslash and a newline get spliced together with the following line

// According to GCC it's only a warning to have whitespace between the
// backslash and the newline...

const char* match_splice(const char* text, void* ctx) {
  using ws = Char<' ','\t'>;
  using match = Seq< Char<'\\'>, Any<ws>, Char<'\n'> >;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------

const char* match_bom(const char* text, void* ctx) {
  if (!text) return nullptr;

  if (uint8_t(text[0]) == 0xFE &&
      uint8_t(text[1]) == 0xFF) {
    return text + 2;
  }

  if (uint8_t(text[0]) == 0xEF &&
      uint8_t(text[1]) == 0xBB &&
      uint8_t(text[2]) == 0xBF) {
    return text + 3;
  }

  return nullptr;
}

//------------------------------------------------------------------------------

const char* match_utf8(const char* text, void* ctx) {
  using ext       = Range<0x80, 0xBF>;
  using onebyte   = Range<0x00, 0x7F>;
  using twobyte   = Seq<Range<0xC0, 0xDF>, ext>;
  using threebyte = Seq<Range<0xE0, 0xEF>, ext, ext>;
  using fourbyte  = Seq<Range<0xF0, 0xF7>, ext, ext, ext>;

  //using match = Oneof<onebyte, twobyte, threebyte, fourbyte>;
  using match = Oneof<twobyte, threebyte, fourbyte>;

  return match::match(text, ctx);
}

//------------------------------------------------------------------------------

const char* match_space(const char* text, void* ctx) {
  using ws = Char<' ','\t'>;
  using match = Some<ws>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------

const char* match_newline(const char* text, void* ctx) {
  using match = Seq<Opt<Char<'\r'>>, Char<'\n'>>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------

const char* match_indentation(const char* text, void* ctx) {
  using match = Seq<Char<'\n'>, Any<Char<' ', '\t'>>>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
// Multi-character character literals are allowed by spec? But
// implementation-defined...

const char* match_escape(const char* text, void* ctx);

const char* match_char_literal(const char* text, void* ctx) {
  using tick = Char<'\''>;
  using escaped = Ref<match_escape>;
  using match = Seq<tick, Some<Oneof<escaped, NotChar<'\''>>>, tick>;
  return match::match(text, ctx);
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
  using trigraphs = Trigraphs<"...<<=>>=">;
  using digraphs  = Digraphs<"---=->!=*=/=&&&=##%=^=+++=<<<===>=>>|=||">;
  using unigraphs = Chars<"-,;:!?.()[]{}*/&#%^+<=>|~">;
  using match = Oneof<trigraphs, digraphs, unigraphs>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
// Single-line comments

const char* match_oneline_comment(const char* text, void* ctx) {
  using match  = Seq<Lit<"//">, Any<NotEOL>, And<EOL>>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
// Multi-line non-nested comments

const char* match_multiline_comment(const char* text, void* ctx) {
  // not-nested version
  using ldelim = Lit<"/*">;
  using rdelim = Lit<"*/">;
  using item   = Seq<Not<rdelim>, Char<>>;
  using match  = Seq<ldelim, Any<item>, rdelim>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
// Multi-line nested comments
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

const char* match_ws(const char* text, void* ctx) {
  using ws = Char<' ','\t','\r','\n'>;
  return ws::match(text, ctx);
}

//------------------------------------------------------------------------------
// 6.4.5 String literals

// encoding-prefix: u8 | u | U | L
// string-literal: encoding-prefixopt " s-char-sequenceopt "
// s-char-sequence: s-char | s-char-sequence s-char
// s-char: any member of the source character set except the double-quote ", backslash \, or new-line character |
//         escape-sequence


// FIXME should be stricter about characters allowed in strings...

const char* match_string(const char* text, void* ctx) {
  using prefix  = Oneof<Lit<"u8">, Char<'u'>, Char<'U'>, Char<'L'>>;
  using quote   = Char<'"'>;
  using escaped = Seq<Char<'\\'>, Char<>>;
  using item    = Oneof<Ref<match_splice>, escaped, NotChar<'"'>>;
  using match   = Seq<Opt<prefix>, quote, Any<item>, quote>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------

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
  // GCC allows dollar signs in identifiers?

const char* match_identifier(const char* text, void* ctx) {
  using lower = Range<'a', 'z'>;
  using upper = Range<'A', 'Z'>;
  using digit = Range<'0', '9'>;
  using item  = Oneof<lower, upper, digit, Char<'_'>, Char<'$'>>;
  using match = Seq<Not<digit>, Some<item>>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
// Matches C #include paths such as "foo/bar/file.txt" or <foo/bar/file.txt>
// FIXME

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

//------------------------------------------------------------------------------
// Matches all lines starting with '#'

const char* match_preproc(const char* text, void* ctx) {

  // Raw strings in preprocs can make a preproc span multiple lines...?
  using item = Oneof<Ref<match_raw_string>, NotEOL>;

  using match = Seq<Char<'#'>, Any<item>, And<EOL>>;
  return match::match(text, ctx);
}

//------------------------------------------------------------------------------
// Matches all valid C integer constants, including embedded tick mark
// separators

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
// Matches all valid C floating point constants. Does not support ' separators.

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
// FIXME redo the "all possible escape sequences" matcher

// escape-sequence:
//   simple-escape-sequence
//   octal-escape-sequence
//   hexadecimal-escape-sequence
//   universal-character-name

// simple-escape-sequence:
//   \’ \" \? \\ \a \b \f \n \r \t \v

// octal-escape-sequence:
//   \ octal-digit
//   \ octal-digit octal-digit
//   \ octal-digit octal-digit octal-digit

// hexadecimal-escape-sequence:
//   \x hexadecimal-digit
//   hexadecimal-escape-sequence hexadecimal-digit

// universal-character-name:
//   \u hex-quad
//   \U hex-quad hex-quad
// hex-quad:
//   hexadecimal-digit hexadecimal-digit hexadecimal-digit hexadecimal-digit

// hexadecimal-digit: 0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F

// nonzero-digit: 1 2 3 4 5 6 7 8 9

const char* match_escape(const char* text, void* ctx) {
  using octal_digit   = Range<'0','7'>;
  using escaped_octal = Seq<Char<'\\'>, octal_digit, octal_digit, octal_digit>;
  using escaped_char  = Seq<Char<'\\'>, Chars<"0?abfnrvt\\\'">>;

  using hex_digit   = Oneof<Range<'0','9'>, Range<'a','f'>, Range<'A','F'>>;
  using escaped_uni = Seq<Char<'\\'>, Char<'u'>, Rep<4, hex_digit>>;

  using match = Oneof<escaped_uni, escaped_octal, escaped_char>;
  return match::match(text, ctx);
}

#if 0
std::optional<cspan> Parser::take_escape_seq() {
  start_span();

  switch (*cursor) {
    case '\'':
    case '"':
    case '?':
    case '\\':
    case '\a':
    case '\b':
    case '\f':
    case '\n':
    case '\r':
    case '\t':
    case '\v':
      take(*cursor);
      return take_top_span();
  }

  auto bookmark = cursor;

  // \nnn
  if (take_digits(8, 3)) return take_top_span();
  else cursor = bookmark;

  // \o{n..}
  if (take("o{") && take_digits(8) && take("}")) return take_top_span();
  else cursor = bookmark;

  // \xn..
  if (take("x") && take_digits(16)) return take_top_span();
  else cursor = bookmark;

  // \x{n..}
  if (take("x{") && take_digits(16) && take("}")) return take_top_span();
  else cursor = bookmark;

  // \unnnn
  if (take("u") && take_digits(16, 4)) return take_top_span();
  else cursor = bookmark;

  // \u{n..}
  if (take("u{") && take_digits(16) && take("}")) return take_top_span();
  else cursor = bookmark;

  // \Unnnnnnnn
  if (take("U") && take_digits(16, 8)) return take_top_span();
  else cursor = bookmark;

  // \N{name}
  if (take("N{") && take_until('}', 1) && take('}')) return take_top_span();
  else cursor = bookmark;

  return drop_span();
}
#endif

/* multiline comment */

float blah = 1.02345e10f;

// spliced line
float blee = 12345678 + \
   912045;

const char* raw_stuff = R"(This is a raw string)";

char asdf = '\1';

/*
keywords:
  auto  break  case  char  const  continue  default  do  double  else  enum
  extern  float  for  goto  if  inline  int  long  register  restrict  return
  short  signed  sizeof  static  struct  switch  typedef  union  unsigned  void
  volatile  while  _Alignas  _Alignof  _Atomic  _Bool  _Complex  _Generic
  _Imaginary  _Noreturn  _Static_assert  _Thread_local


from latest draft:
alignas alignof auto bool break case char const constexpr continue default do
double else enum extern false float for goto if inline int long nullptr
register restrict return short signed sizeof static static_assert struct switch
thread_local true typedef typeof typeof_unqual union unsigned void volatile
while

_Atomic _BitInt _Complex _Decimal128 _Decimal32 _Decimal64 _Generic _Imaginary
_Noreturn
*/

#if 0
// This should be ifd out

int blah() {
}

#endif

//------------------------------------------------------------------------------
