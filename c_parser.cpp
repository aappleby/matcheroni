#include "Matcheroni.h"

using namespace matcheroni;

void* match_type_name(const char* text, void* ctx);

// FIXME fake
using identifier = Atom<>;

// (6.7.2.4)
using atomic_type_specifier = Seq<
  Lit<"_Atomic">,
  Atom<'('>,
  Ref<match_type_name>,
  Atom<')'>
>;

// (6.7.2)
using type_specifier = Oneof<
  Lit<"void">,
  Lit<"char">,
  Lit<"short">,
  Lit<"int">,
  Lit<"long">,
  Lit<"float">,
  Lit<"double">,
  Lit<"signed">,
  Lit<"unsigned">,
  //Seq<Lit<"_BitInt">, Atom<'('>, exp_const, Atom<')'>>,
  Lit<"bool">,
  Lit<"_Complex">,
  Lit<"_Decimal32">,
  Lit<"_Decimal64">,
  Lit<"_Decimal128">,
  //atomic_type_specifier
  //struct_or_union_specifier
  //enum_specifier
  //typedef_name
  //typeof_specifier
>;

// (6.7.12.1)
using attribute_specifier_sequence = Some<attribute_specifier>;

// (6.7.12.1)
using attribute_specifier = Seq<Lit<"[[">, Any<attribute>, Lit<"]]">>;

// (6.7.12.1)
using attribute = Seq< attribute_token, Opt<attribute_argument_clause> >;

// (6.7.12.1)
using attribute_token = Oneof< standard_attribute, attribute_prefixed_token >;

// (6.7.12.1)
using standard_attribute = identifier;


#if 0
//(6.7.7)
type_name:
  specifier_qualifier_list Opt<abstract_declarator>

// (6.7.7)
abstract_declarator:
  pointer
  Opt<pointer> direct_abstract_declarator



// (6.7.2.1)
specifier_qualifier_list:
  type_specifier_qualifier Opt<attribute_specifier_sequence>
  type_specifier_qualifier specifier_qualifier_list

// (6.7.2.1)
type_specifier_qualifier:
  type_specifier
  type-qualifier
  alignment-specifier






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

// (6.6)
exp_const:
  exp_cond
#endif
