#include "Matcheroni.h"

// from the N4950 C++23 draft spec

/*
using typedef_name = Oneof<identifier, simple_template_id>;

using namespace_name = Oneof<identifier, namespace_alias>;
using namespace_alias = identifier;

using class_name = Oneof<identifier, simple_template_id>;

using enum_name = identifier;

using template_name = identifier;

using n_char = NotAtom<'}','\n'>;

using n_char_sequence = Some<n_char>;

using named_universal_character = Seq<Lit<"\N{">, n_char_sequence, Lit<"}">>;

using hex_quad = Rep<4, hexadecimal_digit>;

using simple_hexadecimal_digit_sequence = Some<hexadecimal_digit>;

using universal_character_name = Oneof<
  Seq<Lit<"\u">, hex_quad>,
  Seq<Lit<"\U">, hex_quad, hex_quad>,
  Seq<Lit<"\u{">, simple_hexadecimal_digit_sequence, Lit<"}">>,
  named_universal_character
>;

preprocessing_token:
  header_name
  import_keyword
  module_keyword
  export_keyword
  identifier
  pp_number
  character_literal
  user_defined_character_literal
  string_literal
  user_defined_string_literal
  preprocessing_op_or_punc
  each non-whitespace character that cannot be one of the above

using token = Oneof<identifier, keyword, literal, operator_or_punctuator>;

using h_char = NotAtom<'\n','>'>;
using h_char_sequence = Some<h_char>;

using q_char = NotAtom<'\n','"'>;
using q_char_sequence = Some<q_char>;

using header_name = Oneof<
  Seq<Atom<'<'>, h-char-sequence, Atom<'>'>>,
  Seq<Atom<'"'>, q-char-sequence, Atom<'"'>>
>;

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

using identifier = Seq<identifier_start, Any<identifier_continue>>;

// FIXME not using XID_Start
using identifier_start = nondigit;

// FIXME not using XID_Continue
using identifier_continue = Oneof<digit, nondigit>;

using nondigit = Oneof<Range<'a','z'>, Range<'A','Z'>, Atom<'_'>>;

using digit = Range<'0','9'>;

using keyword = Oneof<
  OneofLit<keywords>,
  import_keyword,
  module_keyword,
  export_keyword,
>;

const char* keywords[] = {
  "alignas", "alignof", "asm", "auto", "bool", "break", "case", "catch", "char",
  "char8_t", "char16_t", "char32_t", "class", "concept", "const", "consteval",
  "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return",
  "co_yield", "decltype", "default", "delete", "do", "double", "dynamic_cast",
  "else", "enum", "explicit", "export", "extern", "false", "float", "for",
  "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace",
  "new", "noexcept", "nullptr", "operator", "private", "protected", "public",
  "register", "reinterpret_cast", "requires", "return", "short", "signed",
  "sizeof", "static", "static_assert", "static_cast", "struct", "switch",
  "template", "this", "thread_local", "throw", "true", "try", "typedef",
  "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
  "volatile", "wchar_t", "while", nullptr
};

using preprocessing_op_or_punc = Oneof<preprocessing_operator, operator_or_punctuator>;

using preprocessing_operator = Oneof<
  Lit<"#">,
  Lit<"##">,
  Lit<"%:">,
  Lit<"%:%:">,
>;

operator-or-punctuator: one of
  { } [ ] ( )
  <: :> <% %> ; : ...
  ? :: . .* -> ->* ~
  ! + - * / % ^ & |
  = += -= *= /= %= ^= &= |=
  == != < > <= >= <=> && ||
  << >> <<= >>= ++ -- ,
  and or xor not bitand bitor compl
  and_eq or_eq xor_eq not_eq

using literal = Oneof<
  integer_literal,
  character_literal,
  floating_point_literal,
  string_literal,
  boolean_literal,
  pointer_literal,
  user_defined_literal,
>;

using integer_literal = Oneof<
  Seq<binary_literal, Opt<integer_suffix>>,
  Seq<octal_literal, Opt<integer_suffix>>,
  Seq<decimal_literal, Opt<integer_suffix>>,
  Seq<hexadecimal_literal, Opt<integer_suffix>>,
>;

template<typename M>
struct Ticked {
  static const char* match(const char* text, void* ctx) {
    if (*text == '\'') text++;
    return M::match(text, ctx);
  }
};

using binary_literal = Seq<Oneof<Lit<"0b">, Lit<"0B">>, binary_digit, Any<ticked_binary_digit>>;

using octal_literal = Seq<Atom<'0'>, Any<ticked_octal_digit>>;

using decimal_literal = Seq<nonzero_digit, Any<ticked_decimal_digit>>;

using hexadecimal_literal = Seq<hexadecimal_prefix, hexadecimal_digit_sequence>;

using hexadecimal_digit_sequence = Seq<hexadecimal_digit, Any<ticked_hexadecimal_digit>>;

using hexadecimal_digit = Oneof<Range<'0','9'>, Range<'a','f'>, Range<'A','F'>>;

using integer_suffix = Oneof<
  Seq<unsigned_suffix, Opt<long_suffix>>
  Seq<unsigned_suffix, Opt<long_long-suffix>>
  Seq<unsigned_suffix, Opt<size_suffix>>
  Seq<long_suffix, Opt<unsigned_suffix>>
  Seq<long_long_suffix, Opt<unsigned_suffix>>
  Seq<size_suffix, Opt<unsigned_suffix>>
>;

using unsigned_suffix = Oneof<Lit<"u">, Lit<"U">>

using long_suffix = Oneof<Lit<"l">, Lit<"L">>;

using long_long_suffix = oneof<Lit<"ll", Lit<"LL">>;

using size_suffix = Oneof<Lit<"z">, Lit<"Z">>;

using character_literal = Seq<Opt<encoding_prefix>, Lit<"'">, c_char_sequence, Lit<"'">>;

using encoding_prefix = Oneof<Lit<"u8">, Lit<"u">, Lit<"U">, Lit<"L">>

using c_char_sequence = Some<c_char>;

using c_char = Oneof<universal_character_name, escape_sequence, basic_c_char>;

using basic_c_char =  NotAtom<'\'', '\\'', '\n'>;

using escape_sequence = Oneof<
  simple_escape_sequence,
  numeric_escape_sequence,
  conditional_escape_sequence
>;

using simple_escape_sequence_char = Chars<'\’', '"', '?', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v'>;
using simple_escape_sequence = Seq<Atom<'\\'>, simple_escape_sequence_char>;

using numeric_escape_sequence = Oneof<octal_escape_sequence, hexadecimal_escape_sequence>;

using simple_octal_digit_sequence = Some<octal_digit>;

using octal_escape_sequence = Oneof<
  Seq<Atom<'\\'>, octal_digit, Opt<octal_digit>, Opt<octal_digit>>,
  Seq<Lit<"\o{">, simple_octal_digit_sequence, Lit<"}">>,
>;

using hexadecimal_escape_sequence = Oneof<
  Seq<Lit<"\x">,  simple_hexadecimal_digit_sequence>,
  Seq<Lit<"\x{">, simple_hexadecimal_digit_sequence, Lit<"}">>,
>;

using conditional_escape_sequence = Seq<Atom<'\\'>, conditional_escape_sequence_char>;

using conditional_escape_sequence_char = Seq<
  Not<
    octal_digit,
    simple_escape_sequence_char,
    Chars<'N','o','u','U','x'>,
  >,
  AnyAtom<char>
>;

using floating_point_literal = Oneof<
  decimal_floating_point_literal,
  hexadecimal_floating_point_literal,
>;

using decimal_floating_point_literal = Oneof<
  Seq<fractional_constant, Opt<exponent_part>, Opt<floating_point_suffix>>,
  Seq<digit_sequence, exponent_part, Opt<floating_point_suffix>>,
>;

using hexadecimal_floating_point_literal = Oneof<
  Seq<hexadecimal_prefix, hexadecimal_fractional_constant, binary_exponent_part, Opt<floating_point_suffix>>,
  Seq<hexadecimal_prefix, hexadecimal_digit_sequence, binary_exponent_part, Opt<floating_point_suffix>>,
>;

using fractional_constant = Oneof<
  Seq<Opt<digit_sequence>, Atom<'.'>, digit_sequence>,
  Seq<digit_sequence, Atom<'.'>>,
>;

using hexadecimal_fractional_constant = Oneof<
  Seq<Opt<hexadecimal_digit_sequence>, Atom<'.'>, hexadecimal_digit_sequence>,
  Seq<hexadecimal_digit_sequence, Atom<'.'>>,
>;

using exponent_part = Oneof<
  Seq<Atom<'e'>, Opt<sign>, digit_sequence>,
  Seq<Atom<'E'>, Opt<sign>, digit_sequence>,
>;

using binary_exponent_part = Oneof<
  Seq<Atom<'p'>, Opt<sign>, digit_sequence>,
  Seq<Atom<'P'>, Opt<sign>, digit_sequence>,
>;

using floating_point_suffix = Oneof<
  Lit<"f">, Lit<"l">, Lit<"F">, Lit<"L">,
  Lit<"f16">, Lit<"f32">, Lit<"f64">, Lit<"f128">,
  Lit<"F16">, Lit<"F32">, Lit<"F64">, Lit<"F128">,
  Lit<"bf16">, Lit<"BF16">>;

using string_literal = Oneof<
  Opt<encoding-prefix>, Atom<'"'>, Opt<s_char_sequence>, Atom<'"'>>.
  Opt<encoding-prefix>, Atom<'R'>, raw_string>,
>;

using s_char_sequence = Some<s_char>;

using s_char = Oneof<universal_character_name, escape_sequence, basic_s_char>

using basic_s_char = NotAtom<'"', '\\', '\n'>;

raw-string:
  " d-char-sequenceopt ( r-char-sequenceopt ) d-char-sequenceopt "

r-char-sequence:
  r-char
  r-char-sequence r-char

r-char:
  any member of the translation character set, except a u+0029 right parenthesis followed by
  the initial d-char-sequence (which may be empty) followed by a u+0022 quotation mark


using d_char = NotAtom<' ', '(', ')', '\\', '\t', '\v', '\f', '\n'>;
using d_char_sequence = Some<d_char>;

using boolean_literal = Oneof<Lit<"false">, Lit<"true">>;

using pointer_literal = Lit<"nullptr">;


using ud_suffix = identifier;

using user-defined-integer-literal = Oneof<
  Seq<decimal_literal,     ud_suffix>,
  Seq<octal_literal,       ud_suffix>,
  Seq<hexadecimal_literal, ud_suffix>,
  Seq<binary_literal,      ud_suffix>,
>;

using user_defined_floating_point_literal = Oneof<
  Seq<fractional_constant, Opt<exponent-part>, ud-suffix>,
  Seq<digit-sequence, exponent-part, ud-suffix>,
  Seq<hexadecimal_prefix, hexadecimal_fractional_constant, binary_exponent_part, ud-suffix>,
  Seq<hexadecimal_prefix, hexadecimal_digit_sequence, binary_exponent_part, ud-suffix>,
>;

using user_defined_string_literal    = Seq<string_literal, ud_suffix>;

using user_defined_character_literal = Seq<character_literal, ud_suffix>;

using user_defined_literal = Oneof<
  user_defined_integer_literal,
  user_defined_floating_point_literal,
  user_defined_string_literal,
  user_defined_character_literal,
>;


*/
