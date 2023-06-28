  #include "Matcheroni.h"
  #include "Lexemes.h"
  #include "c_lexer.h"

// These are not intended to be usable grammar bits, they are just from the C99
// spec for reference.

template<typename P>
using comma_separated = Seq<P, Any<Seq<Atom<','>, P>>, Opt<Atom<','>> >;

template<typename T>
struct RefBase {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    return T::pattern::match(ctx, a, b);
  }
};

struct identifier;
struct constant;
struct string_literal;
struct punctuator;
struct expression;
struct header_name;
struct pp_number;
struct character_constant;


















//------------------------------------------------------------------------------

// A.1 Lexical grammar

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

/*6.4*/ using preprocessing_token =
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

#if 0

// A.1.3 Identifiers
/*6.4.2.1*/ using identifier =
  identifier_nondigit
  identifier identifier_nondigit
  identifier digit
/*6.4.2.1*/ using identifier_nondigit =
  nondigit
  universal_character_name
  other implementation_defined characters

/*6.4.2.1*/
using nondigit = Charset<"_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ">;

/*6.4.2.1*/
using digit = Charset<"0123456789">;

// A.1.4 Universal character names
/*6.4.3*/ using universal_character_name =
  \u hex_quad
  \U hex_quad hex_quad

/*6.4.3*/ using hex_quad =
  hexadecimal_digit hexadecimal_digit
  hexadecimal_digit hexadecimal_digit

// A.1.5 Constants
/*6.4.4*/ using constant =
  integer_constant
  floating_constant
  enumeration_constant
  character_constant

/*6.4.4.1*/ using integer_constant =
  decimal_constant Opt<integer_suffix>
  octal_constant Opt<integer_suffix>
  hexadecimal_constant Opt<integer_suffix>

/*6.4.4.1*/ using decimal_constant =
  nonzero_digit
  decimal_constant digit

/*6.4.4.1*/ using octal_constant =
  0
  octal_constant octal_digit

/*6.4.4.1*/ using hexadecimal_constant =
  hexadecimal_prefix hexadecimal_digit
  hexadecimal_constant hexadecimal_digit

/*6.4.4.1*/
using hexadecimal_prefix =
Oneof<
  Lit<"0x">,
  Lit<"0X">,
>;

/*6.4.4.1*/
using nonzero_digit = Charset<"123456789">;

/*6.4.4.1*/
using octal_digit = Charset<"01234567">;

/*6.4.4.1*/
using hexadecimal_digit = Charset<"0123456789abcdefABCDEF">;

/*6.4.4.1*/ using integer_suffix =
  unsigned_suffix Opt<long_suffix>
  unsigned_suffix long_long_suffix
  long_suffix Opt<unsigned_suffix>
  long_long_suffix Opt<unsigned_suffix>

/*6.4.4.1*/ using unsigned_suffix = one of
  u U

/*6.4.4.1*/ using long_suffix = one of
  l L

/*6.4.4.1*/ using long_long_suffix = one of
  ll LL

/*6.4.4.2*/ using floating_constant =
  decimal_floating_constant
  hexadecimal_floating_constant

/*6.4.4.2*/ using decimal_floating_constant =
  fractional_constant Opt<exponent_part> Opt<floating_suffix>
  digit_sequence exponent_part Opt<floating_suffix>

/*6.4.4.2*/ using hexadecimal_floating_constant =
  hexadecimal_prefix hexadecimal_fractional_constant
  binary_exponent_part Opt<floating_suffix>
  hexadecimal_prefix hexadecimal_digit_sequence
  binary_exponent_part Opt<floating_suffix>

/*6.4.4.2*/ using fractional_constant =
  Opt<digit_sequence> . digit_sequence
  digit_sequence .

/*6.4.4.2*/ using exponent_part =
  e Opt<sign> digit_sequence
  E Opt<sign> digit_sequence

/*6.4.4.2*/ using sign = one of
  + -

/*6.4.4.2*/ using digit_sequence =
  digit
  digit_sequence digit

/*6.4.4.2*/ using hexadecimal_fractional_constant =
  Opt<hexadecimal_digit_sequence> .
  hexadecimal_digit_sequence
  hexadecimal_digit_sequence .

/*6.4.4.2*/ using binary_exponent_part =
  p Opt<sign> digit_sequence
  P Opt<sign> digit_sequence

/*6.4.4.2*/ using hexadecimal_digit_sequence =
  hexadecimal_digit
  hexadecimal_digit_sequence hexadecimal_digit

/*6.4.4.2*/ using floating_suffix = one of
  flFL

/*6.4.4.3*/ using enumeration_constant =
  identifier

/*6.4.4.4*/ using character_constant =
  ' c_char_sequence '
  L' c_char_sequence '

/*6.4.4.4*/ using c_char_sequence =
  c_char
  c_char_sequence c_char

/*6.4.4.4*/ using c_char =
  any member of the source character set except
  the single_quote, backslash \, or new_line character
  escape_sequence

/*6.4.4.4*/ using escape_sequence =
  simple_escape_sequence
  octal_escape_sequence
  hexadecimal_escape_sequence
  universal_character_name

/*6.4.4.4*/ using simple_escape_sequence = one of
  \quote \dquote \? \\
  \a \b \f \n \r \t \v

/*6.4.4.4*/ using octal_escape_sequence =
  \ octal_digit
  \ octal_digit octal_digit
  \ octal_digit octal_digit octal_digit

/*6.4.4.4*/ using hexadecimal_escape_sequence =
  \x hexadecimal_digit
  hexadecimal_escape_sequence hexadecimal_digit

// A.1.6 String literals

/*6.4.5*/ using string_literal =
Oneof<
  Seq<Atom<'"'>,  Opt<s_char_sequence>, Atom<'"'>>,
  Seq<Lit<"L\"">, Opt<s_char_sequence>, Atom<'"'>>,
>;

/*6.4.5*/ using s_char_sequence =
  s_char
  s_char_sequence s_char

/*6.4.5*/ using s_char =
  any member of the source character set except
  the double_quote dquote, backslash \, or new_line character
  escape_sequence


// A.1.7 Punctuators
/*6.4.6*/ using punctuator = one of
  [](){}.->
  ++ -- & * + - ~ !
  /%<< >><><= >= == !=^|&& ||
  ?:;...
  = *= /= %= += -= <<= >>= &= ^= |=
  ,###
  <: :> <% %> %: %:%:

// A.1.8 Header names
/*6.4.7*/
using header_name = Oneof<
  Seq<Atom<'<'>, h_char_sequence, Atom<'>'>>,
  Seq<Atom<'"'>, q_char_sequence, Atom<'"'>>,
>;

/*6.4.7*/
using h_char_sequence = Some<h_char>;

/*6.4.7*/
using h_char = NotAtom<'\n', '>'>;

/*6.4.7*/
using q_char_sequence = Some<q_char>;

/*6.4.7*/
using q_char = NotAtom<'\n', '"'>;

// A.1.9 Preprocessing numbers
/*6.4.8*/
using pp_number_suffix = Oneof<
  digit,
  identifier_nondigit,
  Seq<Atom<'e'>, sign>,
  Seq<Atom<'E'>, sign>,
  Seq<Atom<'p'>, sign>,
  Seq<Atom<'P'>, sign>,
  Seq<Atom<'.'>>,
>;

using pp_number =
Seq<
  Oneof<
    digit,
    Seq<Atom<'.'>, digit>,
  >,
  Any<pp_number_suffix>
>;

#endif
















//------------------------------------------------------------------------------
// A.2 Phrase structure grammar

// A.2.1 Expressions

/*6.5.1*/ using primary_expression =
Oneof<
  identifier,
  constant,
  string_literal,
  Seq<Atom<'('>, expression, Atom<')'>>
>;

#if 0

/*6.5.2*/ using postfix_expression =
Oneof<
  primary_expression
  Seq<postfix_expression [ expression ] >
  Seq<postfix_expression Atom<'('> Opt<argument_expression_list> Atom<')'> >
  Seq<postfix_expression Atom<"."> identifier>
  Seq<postfix_expression Atom<"->"> identifier>
  Seq<postfix_expression Atom<"++"> >
  Seq<postfix_expression Atom<"--"> >
  Seq<Atom<'('>, type_name, Atom<')'>, Atom<'{'>, initializer_list, Atom<'}'>>,
  Seq<Atom<'('>, type_name, Atom<')'>, Atom<'{'>, initializer_list, Atom<'}'>>,
>;

/*6.5.2*/ using argument_expression_list =
  assignment_expression
  argument_expression_list Atom<','> assignment_expression
/*6.5.3*/ using unary_expression =
  postfix_expression
  ++ unary_expression
  -- unary_expression
  unary_operator cast_expression
  sizeof unary_expression
  sizeof Atom<'('> type_name Atom<')'>

/*6.5.3*/ using unary_operator = one of
  &*+-~!

/*6.5.4*/ using cast_expression =
  unary_expression
  Atom<'('> type_name Atom<')'> cast_expression

/*6.5.5*/ using multiplicative_expression =
  cast_expression
  multiplicative_expression * cast_expression
  multiplicative_expression / cast_expression
  multiplicative_expression % cast_expression

/*6.5.6*/ using additive_expression =
  multiplicative_expression
  additive_expression + multiplicative_expression
  additive_expression - multiplicative_expression

/*6.5.7*/ using shift_expression =
  additive_expression
  shift_expression << additive_expression
  shift_expression >> additive_expression

/*6.5.8*/ using relational_expression =
  shift_expression
  relational_expression < shift_expression
  relational_expression > shift_expression
  relational_expression <= shift_expression
  relational_expression >= shift_expression

/*6.5.9*/ using equality_expression =
  relational_expression
  equality_expression == relational_expression
  equality_expression != relational_expression

/*6.5.10*/ using AND_expression =
  equality_expression
  AND_expression & equality_expression

/*6.5.11*/ using exclusive_OR_expression =
  AND_expression
  exclusive_OR_expression ^ AND_expression

/*6.5.12*/ using inclusive_OR_expression =
  exclusive_OR_expression
  inclusive_OR_expression | exclusive_OR_expression

/*6.5.13*/ using logical_AND_expression =
  inclusive_OR_expression
  logical_AND_expression && inclusive_OR_expression

/*6.5.14*/ using logical_OR_expression =
  logical_AND_expression
  logical_OR_expression || logical_AND_expression

/*6.5.15*/ using conditional_expression =
  logical_OR_expression
  logical_OR_expression ? expression Atom<':'> conditional_expression

/*6.5.16*/ using assignment_expression =
  conditional_expression
  unary_expression assignment_operator assignment_expression

/*6.5.16*/ using assignment_operator = one of
  = *= /= %= += -= <<= >>= &= ^= |=

/*6.5.17*/ using expression =
  assignment_expression
  expression Atom<','> assignment_expression

/*6.6*/ using constant_expression =
  conditional_expression
#endif






















//------------------------------------------------------------------------------
// A.2.2 Declarations

struct constant_expression;
struct enumeration_constant;
struct assignment_expression;

/*6.7.3*/ using type_qualifier =
Oneof<
  Keyword<"const">,
  Keyword<"restrict">,
  Keyword<"volatile">
>;

/*6.7.2.1*/ using struct_or_union =
Oneof<
  Keyword<"struct">,
  Keyword<"union">
>;

/*6.7.1*/ using storage_class_specifier =
Oneof<
  Keyword<"typedef">,
  Keyword<"extern">,
  Keyword<"static">,
  Keyword<"auto">,
  Keyword<"register">
>;

/*6.7.2.2*/ using enumerator =
Oneof<
  enumeration_constant,
  Seq<enumeration_constant, Atom<'='>, constant_expression>
>;

/*6.7.2.2*/ using enumerator_list = // Original was recursive
comma_separated<enumerator>;

/*6.7.2.2*/ using enum_specifier =
Opt<
  Seq<Keyword<"enum">, Opt<identifier>, Atom<'{'>, enumerator_list, Atom<'}'>>,
  Seq<Keyword<"enum">, Opt<identifier>, Atom<'{'>, enumerator_list, Atom<','>, Atom<'}'>>,
  Seq<Keyword<"enum">, identifier>
>;

/*6.7.7*/ using typedef_name = identifier;

struct struct_or_union_specifier;

/*6.7.2*/
struct type_specifier {
  using pattern =
  Oneof<
    Keyword<"void">,
    Keyword<"char">,
    Keyword<"short">,
    Keyword<"int">,
    Keyword<"long">,
    Keyword<"float">,
    Keyword<"double">,
    Keyword<"signed">,
    Keyword<"unsigned">,
    Keyword<"_Bool">,
    Keyword<"_Complex">,
    struct_or_union_specifier,
    enum_specifier,
    typedef_name
  >;
};

/*6.7.4*/ using function_specifier =
Keyword<"inline">;

/*6.7*/ using declaration_specifiers =
Some<
  storage_class_specifier,
  type_specifier,
  type_qualifier,
  function_specifier
>;

struct declarator;
struct initializer;

/*6.7*/ using init_declarator =
Oneof<
  declarator,
  Seq<declarator, Atom<'='>, initializer>
>;

/*6.7*/ using init_declarator_list = // original was recursive
comma_separated<init_declarator>;

/*6.7*/
using declaration =
Seq<
  declaration_specifiers,
  Opt<init_declarator_list>,
  Atom<';'>
>;

/*6.7.2.1*/
using specifier_qualifier_list = Some<type_specifier, type_qualifier>;

/*6.7.2.1*/ using struct_declarator =
Oneof<
  declarator,
  Seq<Opt<declarator>, Atom<':'>, constant_expression>
>;

/*6.7.2.1*/
using struct_declarator_list = comma_separated<struct_declarator>;

/*6.7.2.1*/
using struct_declaration = Seq<
  specifier_qualifier_list,
  struct_declarator_list,
  Atom<';'>
>;

/*6.7.2.1*/
using struct_declaration_list = Some<struct_declaration>;

/*6.7.2.1*/
struct struct_or_union_specifier : public RefBase<struct_or_union_specifier> {
  using pattern = Oneof<
    Seq<struct_or_union, Opt<identifier>, Atom<'{'>, struct_declaration_list, Atom<'}'>>,
    Seq<struct_or_union, identifier>
  >;
};

/*6.7.5*/ using type_qualifier_list = // Original was recursive
Some<type_qualifier>;

struct abstract_declarator;

/*6.7.5*/ using parameter_declaration =
Oneof<
  Seq<declaration_specifiers, declarator>,
  Seq<declaration_specifiers, Opt<abstract_declarator>>
>;

/*6.7.5*/ using parameter_list = // original was recursive
comma_separated<parameter_declaration>;

/*6.7.5*/
using parameter_type_list =
Oneof<
  parameter_list,
  Seq<parameter_list, Atom<','>, Keyword<"...">>
>;

/*6.7.5*/
using identifier_list = comma_separated<identifier>;

/*6.7.5 - direct-declarator - Had to tear this one apart a bit. */

using direct_declarator_suffix =
Oneof<
  Seq<Atom<'['>, Opt<type_qualifier_list>, Opt<assignment_expression>, Atom<']'>>,
  Seq<Atom<'['>, Keyword<"static">, Opt<type_qualifier_list>, assignment_expression, Atom<']'>>,
  Seq<Atom<'['>, type_qualifier_list, Keyword<"static">, assignment_expression, Atom<']'>>,
  Seq<Atom<'['>, Opt<type_qualifier_list>, Atom<'*'>, Atom<']'>>,
  Seq<Atom<'('>, parameter_type_list, Atom<')'>>,
  Seq<Atom<'('>, Opt<identifier_list>, Atom<')'>>
>;

using direct_declarator =
Seq<
  Oneof<
    identifier,
    Seq<Atom<'('>, declarator, Atom<')'>>,
    Seq<Atom<'['>, Opt<type_qualifier_list>, Opt<assignment_expression>, Atom<']'>>,
    Seq<Atom<'['>, Keyword<"static">, Opt<type_qualifier_list>, assignment_expression, Atom<']'>>,
    Seq<Atom<'['>, type_qualifier_list, Keyword<"static">, assignment_expression, Atom<']'>>,
    Seq<Atom<'['>, Opt<type_qualifier_list>, Atom<'*'>, Atom<']'>>,
    Seq<Atom<'('>, parameter_type_list, Atom<')'>>,
    Seq<Atom<'('>, Opt<identifier_list>, Atom<')'>>
  >,
  Any<direct_declarator_suffix>
>;

struct pointer;

/*6.7.5*/
struct declarator : public RefBase<declarator> {
  using pattern = Seq<Opt<pointer>, direct_declarator>;
};

/*6.7.5*/
struct pointer : public RefBase<pointer> {
  using pattern = Oneof<
    Seq<Atom<'*'>, Opt<type_qualifier_list>>,
    Seq<Atom<'*'>, Opt<type_qualifier_list>, pointer>
  >;
};

/*6.7.6*/
using type_name = Seq<specifier_qualifier_list, Opt<abstract_declarator>>;

/*6.7.6*/
using direct_abstract_declarator_suffix = Oneof<
  Seq<Atom<'['>, Opt<type_qualifier_list>, Opt<assignment_expression>, Atom<']'>>,
  Seq<Atom<'['>, Keyword<"static">, Opt<type_qualifier_list>, assignment_expression, Atom<']'>>,
  Seq<Atom<'['>, type_qualifier_list, Keyword<"static">, assignment_expression, Atom<']'>>,
  Seq<Atom<'['>, Atom<'*'>, Atom<']'>>,
  Seq<Atom<'('>, Opt<parameter_type_list>, Atom<')'>>
>;

using direct_abstract_declarator =
Seq<
  Oneof<
    Seq<Atom<'('>, abstract_declarator, Atom<')'>>
  >,
  Any<direct_abstract_declarator_suffix>
>;

/*6.7.6*/
struct abstract_declarator : public RefBase<abstract_declarator> {
  using pattern = Oneof<
    pointer,
    Seq<Opt<pointer>, direct_abstract_declarator>
  >;
};

struct initializer_list;

/*6.7.8*/
struct initializer : public RefBase<initializer> {
  using pattern = Oneof<
    assignment_expression,
    Seq<Atom<'{'>, initializer_list, Atom<'}'>>,
    Seq<Atom<'{'>, initializer_list, Atom<','>, Atom<'}'>>
  >;
};

/*6.7.8*/
using designator =
Oneof<
  Seq<Atom<'['>, constant_expression, Atom<']'>>,
  Seq<Atom<'.'>, identifier>
>;

/*6.7.8*/
using designator_list = Some<designator>;

/*6.7.8*/
using designation = Seq<designator_list, Atom<'='>>;

/*6.7.8*/
struct initializer_list : public RefBase<initializer_list> {
  using pattern = Oneof<
    Seq<Opt<designation>, initializer>,
    Seq<initializer_list, Atom<','>, Opt<designation>, initializer>
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
using declaration_list = Some<declaration>;

/*6.9.1*/
using function_definition = Seq<
  declaration_specifiers,
  declarator,
  Opt<declaration_list>,
  compound_statement
>;

/*6.9*/
using external_declaration = Oneof<function_definition, declaration>;

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
    if (atom_cmp(a[-1], C) == 0) {
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
    if (atom_cmp(a[-1], C) == 0) {
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
  Seq<Atom<'#'>, Keyword<"define">,  identifier,     lparen, Opt<identifier_list>,                       Atom<')'>, replacement_list, new_line>,
  Seq<Atom<'#'>, Keyword<"define">,  identifier,     lparen, Keyword<"...">,                             Atom<')'>, replacement_list, new_line>,
  Seq<Atom<'#'>, Keyword<"define">,  identifier,     lparen, identifier_list, Atom<','>, Keyword<"...">, Atom<')'>, replacement_list, new_line>
>;

#if 0

/*6.10*/
using preprocessing_file = Opt<group>;

/*6.10*/
using group = Some<group_part>;

/*6.10*/
using group_part =
Oneof<
  if_section,
  control_line,
  text_line,
  Seq<Atom<'#'>, non_directive>
>;

/*6.10*/
using if_section = Seq<if_group, Opt<elif_groups>, Opt<else_group>, endif_line>;

/*6.10*/
using if_group =
Oneof<
  Seq<Atom<'#'>, Keyword<"if">,     constant_expression, new_line, Opt<group>>,
  Seq<Atom<'#'>, Keyword<"ifdef">,  identifier,          new_line, Opt<group>>,
  Seq<Atom<'#'>, Keyword<"ifndef">, identifier,          new_line, Opt<group>>
>;

/*6.10*/
using elif_groups = Some<elif_group>;

/*6.10*/
using elif_group = Seq<Atom<'#'>, Keyword<"elif">, constant_expression, new_line, Opt<group>>;

/*6.10*/
using else_group = Seq<Atom<'#'>, Keyword<"else">, new_line, Opt<group>>;

/*6.10*/
using endif_line = Seq<Atom<'#'>, Keyword<"endif">, new_line>;






#endif
