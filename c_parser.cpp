#include "Matcheroni.h"
#include <assert.h>

using namespace matcheroni;

// Helpers

template<typename item>
using comma_separated_list = Seq<item, Any<Seq<Atom<','>, item>>>;

template<typename item>
using comma_separated_list2 = Seq<
  Any<Seq<item, Atom<','>>>,
  Opt<item>
>;

// Predecls

const char* match_type_specifier_qualifier(const char* a, const char* b, void* ctx);

const char* match_attribute_specifier_sequence(const char* a, const char* b, void* ctx);
using attribute_specifier_sequence = Ref<match_attribute_specifier_sequence>;

// FIXME these are fake
using identifier = Lit<"id">;
using constant_expression = Lit<"constant_expression">;
using string_literal = Lit<"string">;
using assignment_expression = Lit<"assignment_expression">;
using expression = Lit<"expression">;

//------------------------------------------------------------------------------
// 6.7.11 - Static assertions

using static_assert_declaration = Oneof<
  Seq<Lit<"static_assert">, Atom<'('>, constant_expression, string_literal, Atom<')'>, Atom<';'> >,
  Seq<Lit<"static_assert">, Atom<'('>, constant_expression, Atom<')'>, Atom<';'> >
>;

//------------------------------------------------------------------------------
// 6.7.10 Initialization

const char* match_initializer(const char* a, const char* b, void* ctx) {

  using designator = Oneof<
    Seq<Atom<'['>, constant_expression, Atom<']'> >,
    Seq<Atom<'.'>, identifier>
  >;
  using designator_list = Some<designator>;
  using designation = Seq<designator_list, Atom<'='>>;

  using designator_initializer = Seq<
    Opt<designation>,
    Ref<match_initializer>
  >;

  using braced_initializer = Seq<
    Atom<'{'>,
    comma_separated_list2<designator_initializer>,
    Atom<'}'>
  >;

  using initializer = Oneof<
    assignment_expression,
    braced_initializer
  >;
  return initializer::match(a, b, ctx);
};

using initializer = Ref<match_initializer>;


//------------------------------------------------------------------------------
// 6.7.2.2 Enumeration specifiers

//------------------------------------------------------------------------------

const char* match_enum_specifier(const char* a, const char* b, void* ctx) {
  using type_specifier_qualifier = Ref<match_type_specifier_qualifier>;
  using specifier_qualifier_list = Seq<
    Some<type_specifier_qualifier>,
    Opt<attribute_specifier_sequence>
  >;
  using enum_type_specifier = Seq< Atom<':'>, specifier_qualifier_list >;

  using enumeration_constant = identifier;

  using enumerator = Oneof<
    Seq<enumeration_constant, Opt<attribute_specifier_sequence>>,
    Seq<enumeration_constant, Opt<attribute_specifier_sequence>, Atom<'='>, constant_expression>
  >;

  using enumerator_list = comma_separated_list2<enumerator>;

  using enum_specifier = Oneof<
    Seq<Lit<"enum">, Opt<attribute_specifier_sequence>, Opt<identifier>, Opt<enum_type_specifier>, Atom<'{'>, enumerator_list, Atom<'}'>>,
    Seq<Lit<"enum">,                                        identifier,  Opt<enum_type_specifier> >
  >;

  return enum_specifier::match(a, b, ctx);
}

//------------------------------------------------------------------------------
// 6.7.2.1 - Structure and union specifiers

const char* match_declarator(const char* a, const char* b, void* ctx);

const char* match_struct_or_union_specifier(const char* a, const char* b, void* ctx) {
  using struct_or_union = Oneof<Lit<"struct">, Lit<"union">>;

  using declarator = Ref<match_declarator>;

  using bit_field = Seq<declarator, Atom<':'>, constant_expression>;

  using member_declarator = Oneof<declarator, bit_field>;
  using member_declarator_list = Some<member_declarator>;

  using type_specifier_qualifier = Ref<match_type_specifier_qualifier>;
  using specifier_qualifier_list = Seq<
    Some<type_specifier_qualifier>,
    Opt<attribute_specifier_sequence>
  >;

  using member_declaration = Oneof<
    Seq<Opt<attribute_specifier_sequence>, specifier_qualifier_list, Opt<member_declarator_list>, Atom<';'> >,
    static_assert_declaration
  >;
  using member_declaration_list = Some<member_declaration>;

  using struct_or_union_specifier = Oneof<
    Seq<struct_or_union, Opt<attribute_specifier_sequence>, Opt<identifier>, Atom<'{'>, member_declaration_list, Atom<'}'>>,
    Seq<struct_or_union, Opt<attribute_specifier_sequence>, identifier>
  >;

  return struct_or_union_specifier::match(a, b, ctx);
}

//------------------------------------------------------------------------------
// 6.7.2.1

const char* match_type_name(const char* a, const char* b, void* ctx);

const char* match_type_specifier_qualifier(const char* a, const char* b, void* ctx) {
  using type_name = Ref<match_type_name>;

  using bit_int = Seq<Lit<"_BitInt">, Atom<'('>, constant_expression, Atom<')'>>;
  using atomic_type_specifier = Seq<Lit<"_Atomic">, Atom<'('>, type_name, Atom<')'>>;
  using struct_or_union_specifier = Ref<match_struct_or_union_specifier>;
  using enum_specifier = Ref<match_enum_specifier>;
  using typedef_name = identifier;

  using type_specifier_qualifier = Oneof<
    Lit<"const">,
    Lit<"restrict">,
    Lit<"volatile">,
    Lit<"_Atomic">,
    Seq<Lit<"alignas">, Atom<'('>, type_name, Atom<')'> >,
    Seq<Lit<"alignas">, Atom<'('>, constant_expression, Atom<')'> >,
    Lit<"void">,
    Lit<"char">,
    Lit<"short">,
    Lit<"int">,
    Lit<"long">,
    Lit<"float">,
    Lit<"double">,
    Lit<"signed">,
    Lit<"unsigned">,
    Lit<"bool">,
    Lit<"_Complex">,
    Lit<"_Decimal32">,
    Lit<"_Decimal64">,
    Lit<"_Decimal128">,
    bit_int,
    atomic_type_specifier,
    struct_or_union_specifier,
    enum_specifier,
    typedef_name,
    Seq< Lit<"typeof">,        Atom<'('>, Oneof<expression, type_name>, Atom<')'> >,
    Seq< Lit<"typeof_unqual">, Atom<'('>, Oneof<expression, type_name>, Atom<')'> >
  >;

  return type_specifier_qualifier::match(a, b, ctx);
}

using type_specifier_qualifier = Ref<match_type_specifier_qualifier>;

//------------------------------------------------------------------------------

const char* match_declaration_specifiers(const char* a, const char* b, void* ctx) {

  // 6.7.1 Storage-class specifiers
  using storage_class_specifier = Oneof<
    Lit<"auto">,
    Lit<"constexpr">,
    Lit<"extern">,
    Lit<"register">,
    Lit<"static">,
    Lit<"thread_local">,
    Lit<"typedef">
  >;

  // 6.7.4 Function specifiers
  using function_specifier = Oneof<
    Lit<"inline">,
    Lit<"_Noreturn">
  >;
  using declaration_specifier = Oneof<
    storage_class_specifier,
    type_specifier_qualifier,
    function_specifier
  >;

  using declaration_specifiers = Seq<Some<declaration_specifier>, Opt<attribute_specifier_sequence>>;
  return declaration_specifiers::match(a, b, ctx);
}

using declaration_specifiers = Ref<match_declaration_specifiers>;

//------------------------------------------------------------------------------

const char* match_declaration(const char* a, const char* b, void* ctx) {
  using declarator = Ref<match_declarator>;
  using init_declarator = Seq<declarator, Opt<Seq<Atom<'='>, initializer>>>;
  using init_declarator_list = comma_separated_list<init_declarator>;
  using attribute_declaration = Seq<attribute_specifier_sequence, Atom<';'> >;

  using declaration = Oneof<
    Seq<declaration_specifiers, Opt<init_declarator_list>, Atom<';'> >,
    Seq<attribute_specifier_sequence, declaration_specifiers, init_declarator_list, Atom<';'> >,
    static_assert_declaration,
    attribute_declaration
  >;

  return declaration::match(a, b, ctx);
}

using declaration = Ref<match_declaration>;

//------------------------------------------------------------------------------
// 6.7.6 Declarators

const char* match_pointer(const char* a, const char* b, void* ctx) {
  using type_qualifier = Oneof<
    Lit<"const">,
    Lit<"restrict">,
    Lit<"volatile">,
    Lit<"_Atomic">
  >;
  using type_qualifier_list = Some<type_qualifier>;
  using pointer_unit = Seq< Atom<'*'>, Opt<attribute_specifier_sequence>, Opt<type_qualifier_list> >;
  using pointer = Some<pointer_unit>;
  return pointer::match(a, b, ctx);
}

const char* match_direct_declarator(const char* a, const char* b, void* ctx);
using direct_declarator = Ref<match_direct_declarator>;

const char* match_parameter_type_list(const char* a, const char* b, void* ctx);
using parameter_type_list = Ref<match_parameter_type_list>;

//------------------------------------------------------------------------------
// this is probably going to recurse infinitely...

const char* match_direct_declarator(const char* a, const char* b, void* ctx) {

  using type_qualifier = Oneof<
    Lit<"const">,
    Lit<"restrict">,
    Lit<"volatile">,
    Lit<"_Atomic">
  >;
  using type_qualifier_list = Some<type_qualifier>;
  using array_declarator = Oneof<
    Seq< direct_declarator, Atom<'['>, Opt<type_qualifier_list>, Opt<assignment_expression>, Atom<']'> >,
    Seq< direct_declarator, Atom<'['>, Lit<"static">, Opt<type_qualifier_list>, assignment_expression, Atom<']'> >,
    Seq< direct_declarator, Atom<'['>, type_qualifier_list, Lit<"static">, assignment_expression, Atom<']'> >,
    Seq< direct_declarator, Atom<'['>, Opt<type_qualifier_list>, Atom<'*'>, Atom<']'> >
  >;

  using function_declarator = Seq<
    direct_declarator,
    Atom<'('>,
    Opt<parameter_type_list>,
    Atom<')'>
  >;

  using declarator = Ref<match_declarator>;
  using direct_declarator = Oneof<
    Seq<identifier, Opt<attribute_specifier_sequence> >,
    Seq< Atom<'('>, declarator, Atom<')'> >,
    Seq<array_declarator, Opt<attribute_specifier_sequence> >,
    Seq<function_declarator, Opt<attribute_specifier_sequence> >
  >;

  return direct_declarator::match(a, b, ctx);
}

const char* match_declarator(const char* a, const char* b, void* ctx) {
  using pointer = Ref<match_pointer>;
  using declarator = Seq<Opt<pointer>, direct_declarator>;
  return declarator::match(a, b, ctx);
}

using declarator = Ref<match_declarator>;

//------------------------------------------------------------------------------

const char* match_direct_abstract_declarator(const char* a, const char* b, void* ctx);
using direct_abstract_declarator = Ref<match_direct_abstract_declarator>;

// (6.7.7)
const char* match_abstract_declarator(const char* a, const char* b, void* ctx) {
  using pointer = Ref<match_pointer>;
  using abstract_declarator = Oneof<
    pointer,
    Seq<Opt<pointer>, direct_abstract_declarator>
  >;
  return abstract_declarator::match(a, b, ctx);
}

const char* match_type_name(const char* a, const char* b, void* ctx) {
  using type_specifier_qualifier = Ref<match_type_specifier_qualifier>;
  using abstract_declarator = Ref<match_abstract_declarator>;

  using type_name = Seq<
    Some<type_specifier_qualifier>,
    Opt<attribute_specifier_sequence>,
    Opt<abstract_declarator>
  >;

  return type_name::match(a, b, ctx);
}

// this is also gonna recurse infinitely...
const char* match_direct_abstract_declarator(const char* a, const char* b, void* ctx) {
  // (6.7.7)
  using type_qualifier = Oneof<
    Lit<"const">,
    Lit<"restrict">,
    Lit<"volatile">,
    Lit<"_Atomic">
  >;
  using type_qualifier_list = Some<type_qualifier>;

  using array_abstract_declarator = Oneof<
    Seq<Opt<direct_abstract_declarator>, Atom<'['>,                Opt<type_qualifier_list>,                 Opt<assignment_expression>, Atom<']'> >,
    Seq<Opt<direct_abstract_declarator>, Atom<'['>, Lit<"static">, Opt<type_qualifier_list>,                     assignment_expression,  Atom<']'> >,
    Seq<Opt<direct_abstract_declarator>, Atom<'['>,                    type_qualifier_list,  Lit<"static">,      assignment_expression,  Atom<']'> >,
    Seq<Opt<direct_abstract_declarator>, Atom<'['>, Atom<'*'>,                                                                           Atom<']'> >
  >;

  using function_abstract_declarator = Seq<
    Opt<direct_abstract_declarator>,
    Atom<'('>,
    Opt<parameter_type_list>,
    Atom<')'>
  >;

  using abstract_declarator = Ref<match_abstract_declarator>;

  using direct_abstract_declarator = Oneof<
    Seq<Atom<'('>, abstract_declarator, Atom<')'> >,
    Seq<array_abstract_declarator,    Opt<attribute_specifier_sequence> >,
    Seq<function_abstract_declarator, Opt<attribute_specifier_sequence> >
  >;
  return direct_abstract_declarator::match(a, b, ctx);
}

//------------------------------------------------------------------------------

const char* match_parameter_type_list(const char* a, const char* b, void* ctx) {

  using abstract_declarator = Ref<match_abstract_declarator>;

  using declarator = Ref<match_declarator>;
  using parameter_declaration = Oneof<
    Seq< Opt<attribute_specifier_sequence>, declaration_specifiers, declarator >,
    Seq< Opt<attribute_specifier_sequence>, declaration_specifiers, Opt<abstract_declarator> >
  >;

  using parameter_list = Some<parameter_declaration>;

  using parameter_type_list = Oneof<
    parameter_list,
    Seq< parameter_list, Atom<','>, Lit<"..."> >,
    Lit<"...">
  >;

  return parameter_type_list::match(a, b, ctx);
}

//------------------------------------------------------------------------------

using expression_statement = Oneof<
  Seq<Opt<expression>, Atom<';'> >,
  Seq<attribute_specifier_sequence, expression, Atom<';'> >
>;

using label = Oneof<
  Seq<Opt<attribute_specifier_sequence>, identifier, Atom<':'> >,
  Seq<Opt<attribute_specifier_sequence>, Lit<"case">, constant_expression, Atom<':'> >,
  Seq<Opt<attribute_specifier_sequence>, Lit<"default">, Atom<':'> >
>;

const char* match_statement(const char* a, const char* b, void* ctx);
const char* match_compound_statement(const char* a, const char* b, void* ctx);

using statement = Ref<match_statement>;

using secondary_block = statement;

using labeled_statement = Seq<label, statement>;


using compound_statement = Ref<match_compound_statement>;

using selection_statement = Oneof<
  Seq< Lit<"if">, Atom<'('>, expression, Atom<')'>, secondary_block >,
  Seq< Lit<"if">, Atom<'('>, expression, Atom<')'>, secondary_block, Lit<"else">, secondary_block >,
  Seq< Lit<"switch">, Atom<'('>, expression, Atom<')'>, secondary_block >
>;

using jump_statement = Oneof<
  Seq< Lit<"goto">, identifier, Atom<';'> >,
  Seq< Lit<"continue">, Atom<';'> >,
  Seq< Lit<"break">, Atom<';'> >,
  Seq< Lit<"return">, Opt<expression>, Atom<';'> >
>;

using iteration_statement = Oneof<
  Seq< Lit<"while">, Atom<'('>, expression, Atom<')'>, secondary_block >,
  Seq< Lit<"do">, secondary_block, Lit<"while">, Atom<'('>, expression, Atom<')'>, Atom<';'> >,
  Seq< Lit<"for">, Atom<'('>, Opt<expression>, Atom<';'>, Opt<expression>, Atom<';'>, Opt<expression>, Atom<')'>, secondary_block >,
  Seq< Lit<"for">, Atom<'('>, declaration, Opt<expression>, Atom<';'>, Opt<expression>, Atom<')'>, secondary_block >
>;


using primary_block = Oneof<
  compound_statement,
  selection_statement,
  iteration_statement
>;

using unlabeled_statement = Oneof<
  expression_statement,
  Seq<Opt<attribute_specifier_sequence>, primary_block>,
  Seq<Opt<attribute_specifier_sequence>, jump_statement>
>;

const char* match_compound_statement(const char* a, const char* b, void* ctx) {
  using block_item = Oneof<
    declaration,
    unlabeled_statement,
    label
  >;
  using block_item_list = Some<block_item>;
  using compound_statement = Seq< Atom<'{'>, Opt<block_item_list>, Atom<'}'>>;
  return compound_statement::match(a, b, ctx);
}

const char* match_statement(const char* a, const char* b, void* ctx) {
  using statement = Oneof<labeled_statement, unlabeled_statement>;
  return statement::match(a, b, ctx);
}

//------------------------------------------------------------------------------

using function_body = compound_statement;

using function_definition = Seq<
  Opt<attribute_specifier_sequence>,
  declaration_specifiers,
  declarator,
  function_body
>;

using external_declaration = Oneof<function_definition, declaration>;

using translation_unit = Some<external_declaration>;

//------------------------------------------------------------------------------


#if 0


// (6.5.1)
using primary_expression = Oneof<
  identifier,
  constant,
  string_literal,
  Seq< Atom<'('>, expression, Atom<')'> >,
  generic_selection
>;

// (6.5.1.1)
using generic_selection = Seq<
  Lit<"_Generic">,
  Atom<'('>,
  assignment_expression,
  Atom<','>
  generic_assoc_list,
  Atom<')'>
>;

// (6.5.1.1)
using generic_assoc_list = Oneof<
  generic_association,
  Seq<generic_assoc_list, Atom<','>, generic_association>
>;

// (6.5.1.1)
using generic_association = Oneof<
  Seq< type_name, Atom<':'>, assignment_expression >,
  Seq< Lit<"default">, Atom<':'>, assignment_expression >
>;

// (6.5.2)
using postfix_expression = Oneof<
  primary_expression,
  Seq< postfix_expression, Atom<'['>, expression, Atom<']'> >,
  Seq< postfix_expression, Atom<'('>, Opt<argument_expression_list>, Atom<')'> >,
  Seq< postfix_expression, Atom<'.'>, identifier >,
  Seq< postfix_expression, Lit<"->">, identifier >,
  Seq< postfix_expression, Lit<"++"> >,
  Seq< postfix_expression, Lit<"--"> >,
  compound_literal
>;

// (6.5.2)
using argument_expression_list = Seq<
  assignment_expression,
  Opt< Seq<Atom<','>, assignment_expression> >
>;

// (6.5.2.5)
using compound_literal = Seq<
  Atom<'('>,
  Any<storage_class_specifier>,
  type_name,
  Atom<')'>,
  braced_initializer
>;

// (6.5.3)
using unary_operator = Atom<'&', '*', '+', '-', '~', '!'>;

// (6.5.3)
using unary_expression = Oneof<
  postfix_expression,
  Seq< Lit<"++">, unary_expression >,
  Seq< Lit<"--">, unary_expression >,
  Seq< unary_operator, cast_expression >,
  Seq< Lit<"sizeof">, unary_expression >,
  Seq< Lit<"sizeof">, Atom<'('>, type_name, Atom<')'> >,
  Seq< Lit<"alignof">, Atom<'('>, type_name, Atom<')'> >
>;

// (6.5.4)
using cast_expression = Oneof<
  unary_expression,
  Seq< Atom<'('>, type_name, Atom<')'>, cast_expression >
>;

// (6.5.5)
using multiplicative_expression = Oneof<
  cast_expression,
  Seq<multiplicative_expression, Atom<'*'>, cast_expression>,
  Seq<multiplicative_expression, Atom<'/'>, cast_expression>,
  Seq<multiplicative_expression, Atom<'%'>, cast_expression>
>;

// (6.5.6)
using additive_expression = Oneof<
  multiplicative_expression,
  Seq< additive_expression, Atom<'+'>, multiplicative_expression >,
  Seq< additive_expression, Atom<'-'>, multiplicative_expression >
>;

// (6.5.7)
using shift_expression = Oneof<
  additive_expression,
  Seq< shift_expression, Lit<"<<">, additive_expression >,
  Seq< shift_expression, Lit<">>">, additive_expression >
>;

// (6.5.8)
using relational_expression = Oneof<
  shift_expression,
  Seq< relational_expression, Atom<'<'>, shift_expression >,
  Seq< relational_expression, Atom<'>'>, shift_expression >,
  Seq< relational_expression, Lit<"<=">, shift_expression >,
  Seq< relational_expression, Lit<">=">, shift_expression >
>;

//(6.5.9)
using equality_expression = Oneof<
  relational_expression,
  Seq<equality_expression, Lit<"==">, relational_expression>,
  Seq<equality_expression, Lit<"!=">, relational_expression>
>;

// (6.5.10)
using and_expression = Oneof<
  equality_expression
  Seq< and_expression, Atom<'&'>, equality_expression >
>;

// (6.5.11)
using exclusive_or_expression = Oneof<
  and_expression,
  Seq< exclusive_or_expression, Atom<'^'>, and_expression >
>;

// (6.5.12)
using inclusive_or_expression = Oneof<
  exclusive_or_expression,
  Seq<inclusive_or_expression, Atom<'|'>, exclusive_or_expression>
>;

// (6.5.13)
using logical_and_expression = Oneof<
  inclusive_or_expression,
  Seq<logical_and_expression, Lit<"&&">, inclusive_or_expression>
>;

// (6.5.14)
using logical_or_expression = Oneof<
  logical_and_expression,
  Seq<logical_or_expression, Lit<"||">, logical_and_expression>
>;

// (6.5.15)
using conditional_expression = Oneof<
  logical_or_expression,
  Seq<logical_or_expression, Atom<'?'>, expression, Atom<':'>, conditional_expression>
>;

// (6.5.16)
using assignment_operator = Oneof<
  Lit<"=">,
  Lit<"*=">,
  Lit<"/=">,
  Lit<"%=">,
  Lit<"+=">,
  Lit<"-=">,
  Lit<"<<=">,
  Lit<">>=">,
  Lit<"&=">,
  Lit<"^=">,
  Lit<"|=">
>;

// (6.5.16)
using assignment_expression = Oneof<
  conditional_expression
  Seq<unary_expression, assignment_operator, assignment_expression>
>

// (6.5.17)
using expression = Oneof<
  assignment_expression,
  Seq<expression, Atom<','>, assignment_expression>
>

// (6.6)
using constant_expression = conditional_expression;

#endif

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// 6.7.12 - Attributes

// (6.7.12.1)

const char* match_balanced_token(const char* a, const char* b, void* ctx) {
  using balanced_token = Oneof<
    Seq< Atom<'('>, Any<Ref<match_balanced_token>>, Atom<')'> >,
    Seq< Atom<'['>, Any<Ref<match_balanced_token>>, Atom<']'> >,
    Seq< Atom<'{'>, Any<Ref<match_balanced_token>>, Atom<'}'> >,
    NotAtom<'(',')','{','}','[',']'>
  >;
  return balanced_token::match(a, b, ctx);
}
using balanced_token = Ref<match_balanced_token>;

const char* match_attribute_specifier_sequence(const char* a, const char* b, void* ctx) {
  /*
  standard attribute is one of:
  deprecated fallthrough maybe_unused nodiscard noreturn _Noreturn unsequenced reproducible
  */

  using standard_attribute = identifier;
  using attribute_prefix = identifier;
  using attribute_prefixed_token = Seq<attribute_prefix, Lit<"::">, identifier>;
  using attribute_token = Oneof< standard_attribute, attribute_prefixed_token >;
  using attribute_argument_clause = Seq< Atom<'('>, Any<balanced_token>, Atom<')'> >;
  using attribute = Seq< attribute_token, Opt<attribute_argument_clause> >;
  using attribute_specifier = Seq<Lit<"[[">, Any<attribute>, Lit<"]]">>;
  using attribute_specifier_sequence = Some<attribute_specifier>;

  return attribute_specifier_sequence::match(a, b, ctx);
}

//------------------------------------------------------------------------------
