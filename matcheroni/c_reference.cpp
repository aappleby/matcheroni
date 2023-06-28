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
struct expression;

#if 0
// A.1 Lexical grammar

// A.1.1 Lexical elements
/*6.4*/ using token =
  keyword
  identifier
  constant
  string_literal
  punctuator
/*6.4*/ using preprocessing_token =
  header_name
  identifier
  pp_number
  character_constant
  string_literal
  punctuator
  each non_white_space character that cannot be one of the above

// A.1.2 Keywords

/*6.4.1*/ using keyword = one of
$1_$2to break case char const continue default do double else enum extern float
  for goto if inline int long register restrict return short signed sizeof static
  struct switch typedef union unsigned void volatile while _Bool _Complex
  _Imaginary

// A.1.3 Identifiers
/*6.4.2.1*/ using identifier =
  identifier_nondigit
  identifier identifier_nondigit
  identifier digit
/*6.4.2.1*/ using identifier_nondigit =
  nondigit
  universal_character_name
  other implementation_defined characters
/*6.4.2.1*/ using nondigit = one of
  _abcdefghijklm
  nopqrstuvwxyz
  ABCDEFGHIJKLM
  NOPQRSTUVWXYZ
/*6.4.2.1*/ using digit = one of
  0123456789

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
/*6.4.4.1*/ using hexadecimal_prefix = one of
  0x 0X
/*6.4.4.1*/ using nonzero_digit = one of
  123456789
/*6.4.4.1*/ using octal_digit = one of
  01234567
/*6.4.4.1*/ using hexadecimal_digit = one of
  0123456789
  abcdef
  ABCDEF
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
  " Opt<s_char_sequence> "
  L" Opt<s_char_sequence> "
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
/*6.4.7*/ using header_name =
  < h_char_sequence >
  " q_char_sequence "
/*6.4.7*/ using h_char_sequence =
  h_char
  h_char_sequence h_char
/*6.4.7*/ using h_char =
  any member of the source character set except
  the new_line character and >
/*6.4.7*/ using q_char_sequence =
  q_char
  q_char_sequence q_char
/*6.4.7*/ using q_char =
  any member of the source character set except
  the new_line character and dquote

// A.1.9 Preprocessing numbers
/*6.4.8*/
  using pp_number = Oneof<
  digit
  Seq<Atom<'.'>, digit>,
  Seq<pp_number, digit>,
  Seq<pp_number, identifier_nondigit>,
  Seq<pp_number, Atom<'e'>, sign>,
  Seq<pp_number, Atom<'E'>, sign>,
  Seq<pp_number, Atom<'p'>, sign>,
  Seq<pp_number, Atom<'P'>, sign>,
  Seq<pp_number, Atom<'.'>>,
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
  logical_OR_expression ? expression : conditional_expression

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

struct declarator;
struct initializer;
struct type_specifier;
struct struct_or_union_specifier;
struct constant_expression;
struct enumeration_constant;
struct assignment_expression;
struct abstract_declarator;
struct pointer;
struct initializer_list;
struct designation;

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

/*6.7*/ using init_declarator =
Oneof<
  declarator,
  Seq<declarator, Atom<'='>, initializer>
>;

/*6.7*/ using init_declarator_list = // original was recursive
comma_separated<init_declarator>;

/*6.7*/ using declaration =
  Seq< declaration_specifiers, Opt<init_declarator_list>, Atom<';'> >;

/*6.7.2.1*/ using specifier_qualifier_list = // original was recursive
Some<type_specifier, type_qualifier>;

/*6.7.2.1*/ using struct_declarator =
Oneof<
  declarator,
  Seq<Opt<declarator>, Atom<':'>, constant_expression>
>;

/*6.7.2.1*/ using struct_declarator_list = // original was recursive
comma_separated<struct_declarator>;

/*6.7.2.1*/ using struct_declaration =
Seq<specifier_qualifier_list, struct_declarator_list, Atom<';'>>;

/*6.7.2.1*/ using struct_declaration_list = // original was recursive
Some<
  struct_declaration
>;

/*6.7.2.1*/
struct struct_or_union_specifier : public RefBase<struct_or_union_specifier> {
  using pattern = Oneof<
    Seq<struct_or_union, Opt<identifier>, Atom<'{'>, struct_declaration_list, Atom<'}'>>,
    Seq<struct_or_union, identifier>
  >;
};

/*6.7.5*/ using type_qualifier_list = // Original was recursive
Some<type_qualifier>;

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

/*6.7.8*/
struct initializer : public RefBase<initializer> {
  using pattern = Oneof<
    assignment_expression,
    Seq<Atom<'{'>, initializer_list, Atom<'}'>>,
    Seq<Atom<'{'>, initializer_list, Atom<','>, Atom<'}'>>
  >;
};

/*6.7.8*/
struct initializer_list : public RefBase<initializer_list> {
  using pattern = Oneof<
    Seq<Opt<designation>, initializer>,
    Seq<initializer_list, Atom<','>, Opt<designation>, initializer>
  >;
};

#if 0






/*6.7.8*/ using designation =
  designator_list =
/*6.7.8*/ using designator_list =
  designator
  designator_list designator
/*6.7.8*/ using designator =
  [ constant_expression ]
  . identifier
#endif

#if 0
// A.2.3 Statements

/*6.8*/ using statement =
Oneof<
  labeled_statement
  compound_statement
  expression_statement
  selection_statement
  iteration_statement
  jump_statement
>;

/*6.8.1*/ using labeled_statement =
Oneof<
  identifier : statement
  case constant_expression : statement
  default : statement
>;

/*6.8.2*/ using compound_statement =
  Atom<'{'>, Opt<block_item_list> Atom<'}'>;

/*6.8.2*/ using block_item_list =
Oneof<
  block_item
  block_item_list block_item
>;

/*6.8.2*/ using block_item =
Oneof<
  declaration
  statement
>;

/*6.8.3*/ using expression_statement =
  Opt<expression> Atom<';'>

/*6.8.4*/ using selection_statement =
Oneof<
  Keyword<'if'> Atom<'('> expression Atom<')'> statement
  Keyword<'if'> Atom<'('> expression Atom<')'> statement Keyword<'else'> statement
  Keyword<'switch'> Atom<'('> expression Atom<')'> statement
>;

/*6.8.5*/ using iteration_statement =
Oneof<
  Keyword<'while'> Atom<'('> expression Atom<')'> statement
  Keyword<'do'> statement Keyword<'while'> Atom<'('> expression Atom<')'> Atom<';'>
  Keyword<'for'> Atom<'('> Opt<expression> Atom<';'> Opt<expression> Atom<';'> Opt<expression> Atom<')'> statement
  Keyword<'for'> Atom<'('> declaration Opt<expression> Atom<';'> Opt<expression> Atom<')'> statement
>;

/*6.8.6*/ using jump_statement =
Oneof<
  goto identifier Atom<';'>
  continue Atom<';'>
  break Atom<';'>
  return Opt<expression> Atom<';'>
>;

// A.2.4 External definitions
/*6.9*/ using translation_unit =
  external_declaration
  translation_unit external_declaration

/*6.9*/ using external_declaration =
  function_definition
  declaration

/*6.9.1*/ using function_definition =
  declaration_specifiers declarator Opt<declaration_list> compound_statement

/*6.9.1*/ using declaration_list =
  declaration
  declaration_list declaration

#endif



















//------------------------------------------------------------------------------
// A.3 Preprocessing directives

#if 0


/*6.10*/ using preprocessing_file =
  Opt<group>
/*6.10*/ using group =
  group_part
  group group_part
/*6.10*/ using group_part =
  if_section
  control_line
  text_line
  -# non_directive
/*6.10*/ using if_section =
  if_group Opt<elif_groups> Opt<else_group> endif_line
/*6.10*/ using if_group =
  -# if constant_expression new_line Opt<group>
  -# ifdef identifier new_line Opt<group>
  -# ifndef identifier new_line Opt<group>
/*6.10*/ using elif_groups =
  elif_group
  elif_groups elif_group
/*6.10*/ using elif_group =
  -# elif constant_expression new_line Opt<group>
/*6.10*/ using else_group =
  -# else new_line Opt<group>
/*6.10*/ using endif_line =
  -# endif new_line
/*6.10*/ using control_line =
  -# include pp_tokens new_line
  -# define identifier replacement_list new_line
  -# define identifier lparen Opt<identifier_list> Atom<')'>
  replacement_list new_line
  -# define identifier lparen ... Atom<')'> replacement_list new_line
  -# define identifier lparen identifier_list Atom<','> ... Atom<')'>
  replacement_list new_line
  -# undef identifier new_line
  -# line pp_tokens new_line
  -# error Opt<pp_tokens> new_line
  -# pragma Opt<pp_tokens> new_line
  -# new_line
/*6.10*/ using text_line =
  Opt<pp_tokens> new_line
/*6.10*/ using non_directive =
  pp_tokens new_line
/*6.10*/ using lparen =
  a Atom<'('> character not immediately preceded by white_space
/*6.10*/ using replacement_list =
  Opt<pp_tokens>
/*6.10*/ using pp_tokens =
  preprocessing_token
  pp_tokens preprocessing_token
/*6.10*/ using new_line =
  the new_line character



#endif
