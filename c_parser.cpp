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


#if 0


// (6.5.1)
exp_primary:
  identifier
  constant
  lit_string
  ( expression )
  //generic-selection

/*
// (6.5.1.1)
generic-selection:
  _Generic ( exp_assign , generic-assoc-list )

// (6.5.1.1)
generic-assoc-list:
  generic-association
  generic-assoc-list , generic-association

// (6.5.1.1)
generic-association:
  type_name : exp_assign
  default : exp_assign
*/

// (6.5.2)
exp_postfix:
  exp_primary
  exp_postfix [ expression ]
  exp_postfix ( Opt<argument-expression-list> )
  exp_postfix Atom<'.'> identifier
  exp_postfix Lit<"->"> identifier
  exp_postfix Lit<"++">
  exp_postfix Lit<"--">
  lit_compound

// (6.5.2)
argument-expression-list: Seq<exp_assign, Opt<Seq<Atom<','>, exp_assign>>>

// (6.5.2.5)
lit_compound:
  ( Any<storage_class_specifier> type_name ) braced_initializer

// (6.5.3)
exp_unary:
  exp_postfix
  Lit<"++"> exp_unary
  Lit<"--"> exp_unary
  op_unary exp_cast
  sizeof exp_unary
  sizeof ( type_name )
  alignof ( type_name )
// (6.5.3)
op_unary: one of
  & * + - ~ !
// (6.5.4)
exp_cast:
  exp_unary
  ( type_name ) exp_cast
// (6.5.5)
exp_mul:
  exp_cast
  exp_mul * exp_cast
  exp_mul / exp_cast
  exp_mul % exp_cast
// (6.5.6)
exp_add:
  exp_mul
  add_exp + exp_mul
  add_exp - exp_mul
// (6.5.7)
exp_shift:
  add_exp
  exp_shift << add_exp
  exp_shift >> add_exp
// (6.5.8)
exp_rel:
  exp_shift
  exp_rel < exp_shift
  exp_rel > exp_shift
  exp_rel <= exp_shift
  exp_rel >= exp_shift

//(6.5.9)
eq_exp:
  exp_rel
  eq_exp == exp_rel
  eq_exp != exp_rel

// (6.5.10)
exp_bitand:
  eq_exp
  exp_bitand & eq_exp

// (6.5.11)
exp_bitxor:
  exp_bitand
  exp_bitxor ^ exp_bitand

// (6.5.12)
exp_bitor:
  exp_bitxor
  exp_bitor | exp_bitxor

// (6.5.13)
exp_logand:
  exp_bitor
  exp_logand && exp_bitor

// (6.5.14)
exp_logor:
  exp_logand
  exp_logor || exp_logand

// (6.5.15)
exp_cond:
  exp_logor
  exp_logor ? expression : exp_cond

// (6.5.16)
exp_assign:
  exp_cond
  exp_unary op_assign exp_assign

// (6.5.16)
op_assign: one of
  = *= /= %= += -= <<= >>= &= ^= |=

// (6.5.17)
expression:
  exp_assign
  expression , exp_assign

exp_assign
exp_assign , exp_assign
expression , exp_assign , exp_assign

// (6.6)
exp_const:
  exp_cond
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
