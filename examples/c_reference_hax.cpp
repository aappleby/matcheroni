// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"

#include "examples/c_parser/CLexeme.hpp"
#include "examples/SST.hpp"
#include "examples/c_parser/c_constants.hpp"

using namespace matcheroni;

// These are not intended to be usable grammar bits, they are just from the C99
// spec for reference.

template<typename P>
using comma_separated = Seq<P, Any<Seq<Atom<','>, P>>, Opt<Atom<','>> >;

template<StringParam lit>
struct Keyword {
  static_assert(SST<c_keywords>::contains(lit.str_val));

  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(ctx, *a, LEX_KEYWORD)) return nullptr;
    if (atom_cmp(ctx, *a, lit)) return nullptr;
    return a + 1;
  }
};

template<StringParam q>
struct Operator {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    return Lit<q>::match(ctx, a, b);
  }
};

template<typename T>
struct RefBase {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    return T::pattern::match(ctx, a, b);
  }
};

struct string_literal;
struct punctuator;
struct expression;
struct header_name;
struct pp_number;
struct constant;
struct escape_sequence;
struct assignment_expression;
struct declarator_suffix;
struct declarator;
struct initializer_list;
struct type_cast;

















//------------------------------------------------------------------------------
// A.1 Lexical grammar

/*6.4.2.1*/
using nondigit = Charset<"_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ">;

/*6.4.2.1*/
using digit = Charset<"0123456789">;

/*6.4.2.1*/
using identifier_nondigit =
Oneof<
  nondigit
  //universal_character_name
  //other implementation_defined characters
>;

/*6.4.2.1*/
using identifier =
Seq<
  identifier_nondigit,
  Any<identifier_nondigit, digit>
>;

/*6.4.1*/
using keyword =
Oneof<
  Keyword<"auto">,
  Keyword<"break">,
  Keyword<"case">,
  Keyword<"char">,
  Keyword<"const">,
  Keyword<"continue">,
  Keyword<"default">,
  Keyword<"do">,
  Keyword<"double">,
  Keyword<"else">,
  Keyword<"enum">,
  Keyword<"extern">,
  Keyword<"float">,
  Keyword<"for">,
  Keyword<"goto">,
  Keyword<"if">,
  Keyword<"inline">,
  Keyword<"int">,
  Keyword<"long">,
  Keyword<"register">,
  Keyword<"restrict">,
  Keyword<"return">,
  Keyword<"short">,
  Keyword<"signed">,
  Keyword<"sizeof">,
  Keyword<"static">,
  Keyword<"struct">,
  Keyword<"switch">,
  Keyword<"typedef">,
  Keyword<"union">,
  Keyword<"unsigned">,
  Keyword<"void">,
  Keyword<"volatile">,
  Keyword<"while">,
  Keyword<"_Bool">,
  Keyword<"_Complex">,
  Keyword<"_Imaginary">
>;

/*6.4*/
using token =
Oneof<
  keyword,
  identifier,
  constant,
  string_literal,
  punctuator
>;

/*6.4.4.4*/
using c_char =
Oneof<
  NotAtom<'\'', '\\', '\n'>,
  escape_sequence
>;

/*6.4.4.4*/
using c_char_sequence = Some<c_char>;

/*6.4.4.4*/
using character_constant =
Oneof<
  Seq<Atom<'\''>, c_char_sequence, Atom<'\''>>,
  Seq<Lit<"L'">,  c_char_sequence, Atom<'\''>>
>;

/*6.4*/
using preprocessing_token =
Oneof<
  header_name,
  identifier,
  pp_number,
  character_constant,
  string_literal,
  punctuator,
  //each non_white_space character that cannot be one of the above
  Some<NotAtom<' ','\t','\r','\n'>>
>;

/*6.4.4.1*/
using nonzero_digit = Charset<"123456789">;

/*6.4.4.1*/
using octal_digit = Charset<"01234567">;

/*6.4.4.1*/
using hexadecimal_digit = Charset<"0123456789abcdefABCDEF">;

/*6.4.4.1*/
using decimal_constant = Seq<nonzero_digit, Any<digit>>;

/*6.4.4.1*/
using octal_constant = Seq<Lit<"0">, Any<octal_digit>>;

/*6.4.4.1*/
using hexadecimal_prefix = Oneof<Lit<"0x">, Lit<"0X">>;

/*6.4.4.1*/
using hexadecimal_constant = Seq<hexadecimal_prefix, Some<hexadecimal_digit>>;

/*6.4.4.1*/
using unsigned_suffix = Oneof<Atom<'u'>, Atom<'U'>>;

/*6.4.4.1*/
using long_suffix = Oneof<Atom<'l'>, Atom<'L'>>;

/*6.4.4.1*/
using long_long_suffix = Oneof<Lit<"ll">, Lit<"LL">>;

/*6.4.4.1*/ using integer_suffix =
Oneof<
  Seq<unsigned_suffix,  Opt<long_suffix>>,
  Seq<unsigned_suffix,  long_long_suffix>,
  Seq<long_suffix,      Opt<unsigned_suffix>>,
  Seq<long_long_suffix, Opt<unsigned_suffix>>
>;

/*6.4.4.1*/
using integer_constant =
Oneof<
  Seq<decimal_constant, Opt<integer_suffix>>,
  Seq<octal_constant, Opt<integer_suffix>>,
  Seq<hexadecimal_constant, Opt<integer_suffix>>
>;

/*6.4.3*/
using hex_quad =
Seq<
  hexadecimal_digit,
  hexadecimal_digit,
  hexadecimal_digit,
  hexadecimal_digit
>;

/*6.4.3*/
using universal_character_name =
Oneof<
  Seq<Lit<"\\u">, hex_quad>,
  Seq<Lit<"\\U">, hex_quad, hex_quad>
>;

/*6.4.4.2*/
using digit_sequence = Some<digit>;

/*6.4.4.2*/
using fractional_constant =
Seq<
  Seq<Opt<digit_sequence>, Atom<'.'>, digit_sequence>,
  Seq<digit_sequence, Atom<'.'>>
>;

/*6.4.4.2*/
using sign = Atom<'+', '-'>;

/*6.4.4.2*/
using exponent_part =
Oneof<
  Seq<Atom<'e'>, Opt<sign>, digit_sequence>,
  Seq<Atom<'E'>, Opt<sign>, digit_sequence>
>;

/*6.4.4.2*/
using hexadecimal_digit_sequence = Some<hexadecimal_digit>;

/*6.4.4.2*/
using hexadecimal_fractional_constant =
Oneof<
  Seq<Opt<hexadecimal_digit_sequence>, Atom<'.'>>,
  hexadecimal_digit_sequence,
  Seq<hexadecimal_digit_sequence, Atom<'.'>>
>;

/*6.4.4.2*/
using binary_exponent_part =
Oneof<
  Seq<Atom<'p'>, Opt<sign>, digit_sequence>,
  Seq<Atom<'P'>, Opt<sign>, digit_sequence>
>;

/*6.4.4.2*/
using floating_suffix = Charset<"flFL">;

/*6.4.4.3*/
using enumeration_constant = identifier;

/*6.4.4.2*/
using decimal_floating_constant =
Oneof<
  Seq<fractional_constant, Opt<exponent_part>, Opt<floating_suffix>>,
  Seq<digit_sequence, exponent_part, Opt<floating_suffix>>
>;

/*6.4.4.2*/
using hexadecimal_floating_constant =
Seq<
  Seq<hexadecimal_prefix,   hexadecimal_fractional_constant>,
  Seq<binary_exponent_part, Opt<floating_suffix>>,
  Seq<hexadecimal_prefix,   hexadecimal_digit_sequence>,
  Seq<binary_exponent_part, Opt<floating_suffix>>
>;

/*6.4.4.2*/
using floating_constant =
Oneof<
  decimal_floating_constant,
  hexadecimal_floating_constant
>;


/*6.4.4*/
struct constant : public RefBase<constant> {
  using pattern = Oneof<
    integer_constant,
    floating_constant,
    enumeration_constant,
    character_constant
  >;
};

/*6.4.4.4*/
using simple_escape_sequence =
Oneof<
  Seq<Atom<'\\'>, Atom<'\''>>,
  Seq<Atom<'\\'>, Atom<'\"'>>,
  Seq<Atom<'\\'>, Atom<'?'>>,
  Seq<Atom<'\\'>, Atom<'\\'>>,
  Seq<Atom<'\\'>, Atom<'a'>>,
  Seq<Atom<'\\'>, Atom<'b'>>,
  Seq<Atom<'\\'>, Atom<'f'>>,
  Seq<Atom<'\\'>, Atom<'n'>>,
  Seq<Atom<'\\'>, Atom<'r'>>,
  Seq<Atom<'\\'>, Atom<'t'>>,
  Seq<Atom<'\\'>, Atom<'v'>>
>;

/*6.4.4.4*/
using octal_escape_sequence =
Oneof<
  Seq<Atom<'\\'>, octal_digit>,
  Seq<Atom<'\\'>, octal_digit, octal_digit>,
  Seq<Atom<'\\'>, octal_digit, octal_digit, octal_digit>
>;

/*6.4.4.4*/
using hexadecimal_escape_sequence = Seq<Lit<"\\x">, Some<hexadecimal_digit>>;

/*6.4.4.4*/
struct escape_sequence : public RefBase<escape_sequence> {
  using pattern = Oneof<
    simple_escape_sequence,
    octal_escape_sequence,
    hexadecimal_escape_sequence,
    universal_character_name
  >;
};

/*6.4.5*/ using s_char =
Oneof<
  NotAtom<'"', '\\', '\n'>,
  escape_sequence
>;

/*6.4.5*/
using s_char_sequence = Some<s_char>;

/*6.4.5*/
struct string_literal : public RefBase<string_literal> {
  using pattern = Oneof<
    Seq<Atom<'"'>,  Opt<s_char_sequence>, Atom<'"'>>,
    Seq<Lit<"L\"">, Opt<s_char_sequence>, Atom<'"'>>
  >;
};

/*6.4.6*/
struct punctuator : public RefBase<punctuator> {
  using pattern = Oneof<
    Operator<"[">,
    Operator<"]">,
    Operator<"(">,
    Operator<")">,
    Operator<"{">,
    Operator<"}">,
    Operator<".">,
    Operator<"->">,
    Operator<"++">,
    Operator<"--">,
    Operator<"&">,
    Operator<"*">,
    Operator<"+">,
    Operator<"-">,
    Operator<"~">,
    Operator<"!">,
    Operator<"/">,
    Operator<"%">,
    Operator<"<<">,
    Operator<">>">,
    Operator<"<">,
    Operator<">">,
    Operator<"<=">,
    Operator<">=">,
    Operator<"==">,
    Operator<"!=">,
    Operator<"^">,
    Operator<"|">,
    Operator<"&&">,
    Operator<"||">,
    Operator<"?">,
    Operator<":">,
    Operator<";">,
    Operator<"...">,
    Operator<"=">,
    Operator<"*=">,
    Operator<"/=">,
    Operator<"%=">,
    Operator<"+=">,
    Operator<"-=">,
    Operator<"<<=">,
    Operator<">>=">,
    Operator<"&=">,
    Operator<"^=">,
    Operator<"|=">,
    Operator<",">,
    Operator<"#">,
    Operator<"##">,
    Operator<"<:">,
    Operator<":>">,
    Operator<"<%">,
    Operator<"%>">,
    Operator<"%:">,
    Operator<"%:%:">
  >;
};

/*6.4.7*/
using h_char = NotAtom<'\n', '>'>;

/*6.4.7*/
using h_char_sequence = Some<h_char>;

/*6.4.7*/
using q_char = NotAtom<'\n', '"'>;

/*6.4.7*/
using q_char_sequence = Some<q_char>;

/*6.4.7*/
struct header_name : public RefBase<header_name> {
  using pattern = Oneof<
    Seq<Atom<'<'>, h_char_sequence, Atom<'>'>>,
    Seq<Atom<'"'>, q_char_sequence, Atom<'"'>>
  >;
};

// A.1.9 Preprocessing numbers
/*6.4.8*/
using pp_number_suffix = Oneof<
  digit,
  identifier_nondigit,
  Seq<Atom<'e'>, sign>,
  Seq<Atom<'E'>, sign>,
  Seq<Atom<'p'>, sign>,
  Seq<Atom<'P'>, sign>,
  Seq<Atom<'.'>>
>;

struct pp_number : public RefBase<pp_number> {
  using pattern = Seq<
    Oneof<
      digit,
      Seq<Atom<'.'>, digit>
    >,
    Any<pp_number_suffix>
  >;
};
















//------------------------------------------------------------------------------
// A.2 Phrase structure grammar

using expression_prefix =
Oneof<
  Operator<"++">,
  Operator<"--">,
  Keyword<"sizeof">,
  Charset<"&*+-~!">,
  type_cast
>;

using expression_suffix =
Oneof<
  Seq<Atom<'['>, expression, Atom<']'>>,
  Seq<Atom<'('>, Opt<comma_separated<assignment_expression>>, Atom<')'>>,
  Seq<Operator<".">, identifier>,
  Seq<Operator<"->">, identifier>,
  Operator<"++">,
  Operator<"--">
>;

struct unary_expression : public RefBase<unary_expression> {
  using pattern = Seq<
    Any<expression_prefix>,
    Oneof<
      identifier,
      constant,
      string_literal,
      Seq<Atom<'('>, expression, Atom<')'>>,
      initializer_list,
      Seq<Keyword<"sizeof">, type_cast>
    >,
    Any<expression_suffix>
  >;
};

/*6.5.4*/
struct cast_expression : public RefBase<cast_expression> {
  using pattern = Seq<Any<type_cast>, unary_expression>;
};

/*6.5.5*/
struct multiplicative_expression : public RefBase<multiplicative_expression> {
  using pattern = Seq<
    cast_expression,
    Any<
      Seq<Atom<'*'>, cast_expression>,
      Seq<Atom<'/'>, cast_expression>,
      Seq<Atom<'%'>, cast_expression>
    >
  >;
};

/*6.5.6*/
struct additive_expression : public RefBase<additive_expression> {
  using pattern = Seq<
    multiplicative_expression,
    Any<
      Seq<Atom<'+'>, multiplicative_expression>,
      Seq<Atom<'-'>, multiplicative_expression>
    >
  >;
};

/*6.5.7*/
struct shift_expression : public RefBase<shift_expression> {
  using pattern = Seq<
    additive_expression,
    Any<
      Seq<Operator<"<<">, additive_expression>,
      Seq<Operator<">>">, additive_expression>
    >
  >;
};

/*6.5.8*/
struct relational_expression : public RefBase<relational_expression> {
  using pattern = Seq<
    shift_expression,
    Any<
      Seq<Operator<"<">,  shift_expression>,
      Seq<Operator<">">,  shift_expression>,
      Seq<Operator<"<=">, shift_expression>,
      Seq<Operator<">=">, shift_expression>
    >
  >;
};

/*6.5.9*/
struct equality_expression : public RefBase<equality_expression> {
  using pattern = Seq<
    relational_expression,
    Any<
      Seq<Operator<"==">, relational_expression>,
      Seq<Operator<"!=">, relational_expression>
    >
  >;
};

/*6.5.10*/
struct AND_expression : public RefBase<AND_expression> {
  using pattern = Seq<
    equality_expression,
    Any<
      Seq<Operator<"&">, equality_expression>
    >
  >;
};

/*6.5.11*/
struct exclusive_OR_expression : public RefBase<exclusive_OR_expression> {
  using pattern = Seq<
    AND_expression,
    Any<
      Seq<Operator<"^">, AND_expression>
    >
  >;
};

/*6.5.12*/
struct inclusive_OR_expression : public RefBase<inclusive_OR_expression> {
  using pattern = Seq<
    exclusive_OR_expression,
    Any<
      Seq<Operator<"|">, exclusive_OR_expression>
    >
  >;
};

/*6.5.13*/
struct logical_AND_expression : public RefBase<logical_AND_expression> {
  using pattern = Seq<
    inclusive_OR_expression,
    Any<
      Seq<Operator<"&&">, inclusive_OR_expression>
    >
  >;
};

/*6.5.14*/
struct logical_OR_expression : public RefBase<logical_OR_expression> {
  using pattern = Seq<
    logical_AND_expression,
    Any<
      Seq<Operator<"||">, logical_AND_expression>
    >
  >;
};

/*6.5.15*/
struct conditional_expression : public RefBase<conditional_expression> {
  using pattern = Seq<
    logical_OR_expression,
    Any<
      Seq<Operator<"?">, expression, Operator<":">, expression>
    >
  >;
};

/*6.6*/
struct constant_expression : public RefBase<constant_expression> {
  using pattern = conditional_expression;
};

/*6.5.16*/
using assignment_operator =
Oneof<
  Operator<"<<=">,
  Operator<">>=">,
  Operator<"*=">,
  Operator<"/=">,
  Operator<"%=">,
  Operator<"+=">,
  Operator<"-=">,
  Operator<"&=">,
  Operator<"^=">,
  Operator<"|=">,
  Operator<"=">
>;

/*6.5.16*/
struct assignment_expression : public RefBase<assignment_expression> {
  using pattern = Seq<
    conditional_expression,
    Any<
      Seq<assignment_operator, conditional_expression>
    >
  >;
};

/*6.5.17*/
struct expression : public RefBase<expression> {
  using pattern = Seq<
    assignment_expression,
    Any<
      Seq<Atom<','>, assignment_expression>
    >
  >;
};
























//------------------------------------------------------------------------------
// A.2.2 Declarations

// Equivalent to NotEmpty<Seq<Opt<a>, Opt<b>, ...>>

template<typename P, typename... rest>
struct PickSome {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    auto c = Opt<P>::match(ctx, a, b);
    auto d = PickSome<rest...>::match(ctx, c, b);
    if (d) {
      // Rest match was non-empty, succeed
      return d;
    }
    else if (c != a) {
      // First match was non-empty, succeed
      return c;
    }
    else {
      // Total match was empty, fail
      return nullptr;
    }
  }
};

template<typename P>
struct PickSome<P> {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    return P::match(ctx, a, b);
  }
};

using enum_body =
Seq<
  Atom<'{'>,
  comma_separated<
    Seq<
      enumeration_constant,
      Opt<Seq<Atom<'='>, constant_expression>>
    >
  >,
  Atom<'}'>
>;

/*6.7.2.2*/
using enum_specifier =
Seq<
  Keyword<"enum">,
  PickSome<
    identifier,
    enum_body
  >
>;

/*6.7.7*/ using typedef_name = identifier;

struct struct_or_union_specifier;

/*6.7*/ using declaration_specifier =
Oneof<
  Keyword<"auto">, //
  Keyword<"extern">, //
  Keyword<"inline">, //
  Keyword<"register">, //
  Keyword<"static">, //
  Keyword<"typedef">, //

  Keyword<"_Bool">,
  Keyword<"_Complex">,
  Keyword<"char">,
  Keyword<"const">,
  Keyword<"double">,
  Keyword<"float">,
  Keyword<"int">,
  Keyword<"long">,
  Keyword<"restrict">,
  Keyword<"short">,
  Keyword<"signed">,
  Keyword<"unsigned">,
  Keyword<"void">,
  Keyword<"volatile">,
  struct_or_union_specifier,
  enum_specifier,
  typedef_name
>;

/*6.7.2.1*/
using specifier_qualifier =
Oneof<
  Keyword<"_Bool">,
  Keyword<"_Complex">,
  Keyword<"char">,
  Keyword<"const">,
  Keyword<"double">,
  Keyword<"float">,
  Keyword<"int">,
  Keyword<"long">,
  Keyword<"restrict">,
  Keyword<"short">,
  Keyword<"signed">,
  Keyword<"unsigned">,
  Keyword<"void">,
  Keyword<"volatile">,
  struct_or_union_specifier,
  enum_specifier,
  typedef_name
>;


using struct_body =
Seq<
  Atom<'{'>,
  Some<
    Seq<
      Some<specifier_qualifier>,
      comma_separated<
        PickSome<
          declarator,
          Seq<Atom<':'>, constant_expression>
        >
      >,
      Atom<';'>
    >
  >,
  Atom<'}'>
>;


struct struct_or_union_specifier : public RefBase<struct_or_union_specifier> {
  using pattern =
  Seq<
    Oneof<
      Keyword<"struct">,
      Keyword<"union">
    >,
    PickSome<
      identifier,
      struct_body
    >
  >;
};

/*6.7.5*/ using type_qualifier_list =
Some<
  Keyword<"const">,
  Keyword<"restrict">,
  Keyword<"volatile">
>;

using pointer =
Seq<
  Atom<'*'>,
  Opt<type_qualifier_list>
>;

/*6.7*/
using declaration =
Seq<
  Some<declaration_specifier>,
  Opt<comma_separated<
    Seq<
      declarator,
      Opt<
        Atom<'='>,
        Oneof<
          assignment_expression,
          initializer_list
        >
      >
    >
  >>,
  Atom<';'>
>;

/*6.7.6*/
struct abstract_declarator : public RefBase<abstract_declarator> {
  using pattern = Oneof<
    Some<pointer>,
    Seq<
      Seq<Atom<'('>, abstract_declarator, Atom<')'>>,
      Any<declarator_suffix>
    >
  >;
};

/*6.7.5*/
struct declarator : public RefBase<declarator> {
  using pattern = Seq<
    Any<pointer>,
    Oneof<
      identifier,
      Seq<Atom<'('>, declarator, Atom<')'>>
    >,
    Any<
      declarator_suffix,
      Seq<Atom<'('>, comma_separated<identifier>, Atom<')'>>
    >
  >;
};

struct declarator_suffix : public RefBase<declarator_suffix> {
  using pattern = Oneof<
    Seq<Atom<'['>,                                                                                     Atom<']'>>,
    Seq<Atom<'['>,                     type_qualifier_list,                                            Atom<']'>>,
    Seq<Atom<'['>,                                                              assignment_expression, Atom<']'>>,
    Seq<Atom<'['>,                     type_qualifier_list,                     assignment_expression, Atom<']'>>,
    Seq<Atom<'['>, Keyword<"static">,  type_qualifier_list,                     assignment_expression, Atom<']'>>,
    Seq<Atom<'['>, Keyword<"static">,                                           assignment_expression, Atom<']'>>,
    Seq<Atom<'['>,                     type_qualifier_list,  Keyword<"static">, assignment_expression, Atom<']'>>,


    Seq<Atom<'['>, Atom<'*'>, Atom<']'>>,

    Seq<
      Atom<'('>,
      comma_separated<
        Seq<
          Some<declaration_specifier>,
          Opt<declarator, abstract_declarator>
        >
      >,
      Opt<Seq<Atom<','>, Operator<"...">>>,
      Atom<')'>
    >,

    Seq<Atom<'('>, Atom<')'>>
  >;
};

struct type_cast : public RefBase<type_cast> {
  using pattern =
  Seq<
    Atom<'('>,
    Seq<
      Some<specifier_qualifier>,
      Opt<abstract_declarator>
    >,
    Atom<')'>
  >;
};

struct initializer_list : public RefBase<initializer_list> {
  using pattern =
  Seq<
    Atom<'{'>,
    comma_separated<
      Seq<
        Opt<
          Seq<
            Some<
              Seq<Atom<'['>, constant_expression, Atom<']'>>,
              Seq<Atom<'.'>, identifier>
            >,
            Atom<'='>
          >
        >,
        Oneof<
          assignment_expression,
          initializer_list
        >
      >
    >,
    Atom<'}'>
  >;
};

































//------------------------------------------------------------------------------
// A.2.3 Statements

struct statement;

/*6.8.2*/
using block_item = Oneof<declaration, statement>;

/*6.8.2*/
using block_item_list = Some<block_item>;

/*6.8.1*/ using labeled_statement =
Oneof<
  Seq<identifier, Atom<':'>, statement>,
  Seq<Keyword<"case">, constant_expression, Atom<':'>, statement>,
  Seq<Keyword<"default">, Atom<':'>, statement>
>;

/*6.8.2*/
using compound_statement = Seq<Atom<'{'>, Opt<block_item_list>, Atom<'}'>>;

/*6.8.3*/
using expression_statement = Seq<Opt<expression>, Atom<';'>>;

/*6.8.4*/ using selection_statement =
Oneof<
  Seq<Keyword<"if">, Atom<'('>, expression, Atom<')'>, statement>,
  Seq<Keyword<"if">, Atom<'('>, expression, Atom<')'>, statement, Keyword<"else">, statement>,
  Seq<Keyword<"switch">, Atom<'('>, expression, Atom<')'>, statement>
>;

/*6.8.5*/
using iteration_statement = Oneof<
  Seq<Keyword<"while">, Atom<'('>, expression, Atom<')'>, statement>,
  Seq<Keyword<"do">, statement, Keyword<"while">, Atom<'('>, expression, Atom<')'>, Atom<';'>>,
  Seq<Keyword<"for">, Atom<'('>, Opt<expression>, Atom<';'>, Opt<expression>, Atom<';'>, Opt<expression>, Atom<')'>, statement>,
  Seq<Keyword<"for">, Atom<'('>, declaration, Opt<expression>, Atom<';'>, Opt<expression>, Atom<')'>, statement>
>;

/*6.8.6*/
using jump_statement = Oneof<
  Seq<Keyword<"goto">, identifier, Atom<';'>>,
  Seq<Keyword<"continue">, Atom<';'>>,
  Seq<Keyword<"break">, Atom<';'>>,
  Seq<Keyword<"return">, Opt<expression>, Atom<';'>>
>;

/*6.8*/
struct statement : public RefBase<statement> {
  using pattern = Oneof<
    labeled_statement,
    compound_statement,
    expression_statement,
    selection_statement,
    iteration_statement,
    jump_statement
  >;
};

/*6.9.1*/
using function_definition = Seq<
  Some<declaration_specifier>,
  declarator,
  Any<declaration>,
  compound_statement
>;

/*6.9*/
using external_declaration =
Oneof<
  function_definition,
  declaration
>;

// A.2.4 External definitions
/*6.9*/
using translation_unit = Some<external_declaration>;

//------------------------------------------------------------------------------



















//------------------------------------------------------------------------------
// A.3 Preprocessing directives

/*6.10*/
using pp_tokens = Some<preprocessing_token>;

/*6.10*/
using replacement_list = Opt<pp_tokens>;

/*6.10*/
using new_line = Atom<'\n'>;

/*6.10*/
using text_line = Seq<Opt<pp_tokens>, new_line>;

/*6.10*/
using non_directive = Seq<pp_tokens, new_line>;

/*6.10*/
// a Atom<'('> character not immediately preceded by white_space

// FIXME this might be wrong?
template<auto C, auto... rest>
struct NotAfterAtom {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(ctx, a[-1], C) == 0) {
      return nullptr;
    } else {
      return NotAfterAtom<rest...>::match(ctx, a, b);
    }
  }
};

template<auto C>
struct NotAfterAtom<C> {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(ctx, a[-1], C) == 0) {
      return nullptr;
    } else {
      return a;
    }
  }
};

using lparen =
Seq<
  NotAfterAtom<' ', '\r', '\n', '\t'>,
  Atom<'('>
>;

/*6.10*/
using control_line =
Oneof<
  Seq<Atom<'#'>, Keyword<"include">, pp_tokens,      new_line>,
  Seq<Atom<'#'>, Keyword<"undef">,   identifier,     new_line>,
  Seq<Atom<'#'>, Keyword<"line">,    pp_tokens,      new_line>,
  Seq<Atom<'#'>, Keyword<"error">,   Opt<pp_tokens>, new_line>,
  Seq<Atom<'#'>, Keyword<"pragma">,  Opt<pp_tokens>, new_line>,
  Seq<Atom<'#'>,                                     new_line>,
  Seq<Atom<'#'>, Keyword<"define">,  identifier,     replacement_list, new_line>,
  Seq<Atom<'#'>, Keyword<"define">,  identifier,     lparen, Opt<comma_separated<identifier>>,                       Atom<')'>, replacement_list, new_line>,
  Seq<Atom<'#'>, Keyword<"define">,  identifier,     lparen, Keyword<"...">,                                         Atom<')'>, replacement_list, new_line>,
  Seq<Atom<'#'>, Keyword<"define">,  identifier,     lparen, comma_separated<identifier>, Atom<','>, Keyword<"...">, Atom<')'>, replacement_list, new_line>
>;

struct group;

/*6.10*/
using if_group =
Oneof<
  Seq<Atom<'#'>, Keyword<"if">,     constant_expression, new_line, Opt<group>>,
  Seq<Atom<'#'>, Keyword<"ifdef">,  identifier,          new_line, Opt<group>>,
  Seq<Atom<'#'>, Keyword<"ifndef">, identifier,          new_line, Opt<group>>
>;

/*6.10*/
using elif_group = Seq<Atom<'#'>, Keyword<"elif">, constant_expression, new_line, Opt<group>>;

/*6.10*/
using elif_groups = Some<elif_group>;

/*6.10*/
using else_group = Seq<Atom<'#'>, Keyword<"else">, new_line, Opt<group>>;

/*6.10*/
using endif_line = Seq<Atom<'#'>, Keyword<"endif">, new_line>;

/*6.10*/
using preprocessing_file = Opt<group>;

/*6.10*/
using if_section = Seq<if_group, Opt<elif_groups>, Opt<else_group>, endif_line>;

/*6.10*/
using group_part =
Oneof<
  if_section,
  control_line,
  text_line,
  Seq<Atom<'#'>, non_directive>
>;

/*6.10*/
struct group : public RefBase<group> {
  using pattern = Some<group_part>;
};
