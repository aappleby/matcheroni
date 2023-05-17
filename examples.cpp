#include "Matcheroni.h"

using namespace matcheroni;

//------------------------------------------------------------------------------

const char* match_space(const char* text) {
  using ws = Char<' ','\t'>;
  using match = Some<ws>;
  return match::match(text);
}

//------------------------------------------------------------------------------

const char* match_newline(const char* text) {
  using match = Seq<Opt<Char<'\r'>>, Char<'\n'>>;
  return match::match(text);
}

//------------------------------------------------------------------------------

const char* match_indentation(const char* text) {
  using match = Seq<Char<'\n'>, Any<Char<' ', '\t'>>>;
  return match::match(text);
}

//------------------------------------------------------------------------------

const char* match_char_literal(const char* text) {
  using tick = Char<'\''>;
  using escaped = Ref<match_escape>;
  using unescaped = Char<>;
  using match = Seq<tick, Oneof<escaped, unescaped>, tick>;
  return match::match(text);
}

//------------------------------------------------------------------------------

const char* match_punct(const char* text) {
  using trigraphs = Trigraphs<"...<<=>>=">;
  using digraphs  = Digraphs<"---=->!=*=/=&&&=##%=^=+++=<<<===>=>>|=||">;
  using unigraphs = Chars<"-,;:!?.()[]{}*/&#%^+<=>|~">;
  using match = Oneof<trigraphs, digraphs, unigraphs>;
  return match::match(text);
}

//------------------------------------------------------------------------------
// Single-line comments

const char* match_oneline_comment(const char* text) {
  using ldelim = Lit<"//">;
  using rdelim = Oneof<Char<'\n'>, Char<0>>;
  using item   = Seq<Not<rdelim>, Char<>>;
  using match  = Seq<ldelim, Any<item>, And<rdelim>>;
  return match::match(text);
}

//------------------------------------------------------------------------------
// Multi-line non-nested comments

const char* match_multiline_comment(const char* text) {
  // not-nested version
  using ldelim = Lit<"/*">;
  using rdelim = Lit<"*/">;
  using item   = Seq<Not<rdelim>, Char<>>;
  using match  = Seq<ldelim, Any<item>, rdelim>;
  return match::match(text);
}

//------------------------------------------------------------------------------
// Multi-line nested comments

const char* match_nested_comment_body(const char* text);

const char* match_nested_comment(const char* text) {
  using ldelim = Lit<"/*">;
  using rdelim = Lit<"*/">;
  using item   = Seq<Not<ldelim>, Not<rdelim>, Char<>>;
  using body   = Ref<match_nested_comment_body>;
  using match  = Seq<ldelim, body, rdelim>;
  return match::match(text);
}

const char* match_nested_comment_body(const char* text) {
  using ldelim = Lit<"/*">;
  using rdelim = Lit<"*/">;
  using item   = Seq<Not<ldelim>, Not<rdelim>, Char<>>;
  using nested = Ref<match_nested_comment>;
  using match  = Any<Oneof<nested,item>>;
  return match::match(text);
}

//------------------------------------------------------------------------------

const char* match_ws(const char* text) {
  using ws = Char<' ','\t','\r','\n'>;
  return ws::match(text);
}

//------------------------------------------------------------------------------

const char* match_string(const char* text) {
  using quote = Char<'"'>;
  using item  = Oneof< Lit<"\\\"">, NotChar<'"'> >;
  using match = Seq<quote, Any<item>, quote>;
  return match::match(text);
}

//------------------------------------------------------------------------------

const char* match_raw_string(const char* text) {
  using raw_a = Lit<"R\"(">;
  using raw_b = Lit<")\"">;
  using match = Seq<raw_a, Any<Seq<Not<raw_b>, Char<>>>, raw_b>;
  return match::match(text);
}

//------------------------------------------------------------------------------

const char* match_identifier(const char* text) {
  using first = Oneof<Range<'a','z'>, Range<'A','Z'>, Char<'_'>>;
  using rest  = Oneof<Range<'a','z'>, Range<'A','Z'>, Char<'_'>, Range<'0','9'>>;
  using match = Seq<first, Any<rest>>;
  return match::match(text);
}

//------------------------------------------------------------------------------
// Matches C #include paths such as "foo/bar/file.txt" or <foo/bar/file.txt>

const char* match_include_path(const char* text) {
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
  return match::match(text);
}

//------------------------------------------------------------------------------
// Matches all C #<word> preprocessor identifiers. Must be followed by
// whitespace.

const char* match_preproc(const char* text) {
  using item  = Range<'a','z'>;
  using match = Seq< Char<'#'>, Some<item>, And<Char<' ','\t'>> >;
  return match::match(text);
}

//------------------------------------------------------------------------------
// Matches all valid C integer constants, including embedded tick mark
// separators

const char* match_int(const char* text) {
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

  return match::match(text);
}

//------------------------------------------------------------------------------
// Matches all valid C floating point constants. Does not support ' separators.

const char* match_float(const char* text) {
  using digit  = Range<'0','9'>;
  using point  = Char<'.'>;
  using sign   = Char<'+','-'>;
  using exp    = Seq<Char<'e','E'>, Opt<sign>, Some<digit>>;
  using suffix = Char<'f','l','F','L'>;

  using with_dot = Seq<Any<digit>, point, Some<digit>, Opt<exp>, Opt<suffix>>;
  using no_dot   = Seq<Some<digit>, exp, Opt<suffix>>;

  using match = Oneof<with_dot, no_dot>;

  return match::match(text);
}

//------------------------------------------------------------------------------
// FIXME redo the "all possible escape sequences" matcher

const char* match_escape(const char* text) {
  using octal_digit   = Range<'0','7'>;
  using escaped_octal = Seq<Char<'\\'>, octal_digit, octal_digit, octal_digit>;
  using escaped_char  = Seq<Char<'\\'>, Chars<"0?abfnrvt\\\'">>;
  using match = Oneof<escaped_octal, escaped_char>;
  return match::match(text);
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

//------------------------------------------------------------------------------
