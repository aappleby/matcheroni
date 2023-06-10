#include "Matcheroni.h"
#include "Node.h"

#ifdef MATCHERONI_USE_NAMESPACE
using namespace matcheroni;
#endif

using identifier = Atom<NODE_IDENTIFIER>;
using constant = Atom<TOK_CONST>;
using string_literal = Atom<NODE_STRING>;
using lparen = Atom<TOK_LPAREN>;
using rparen = Atom<TOK_RPAREN>;
using lbrace = Atom<TOK_LBRACE>;
using rbrace = Atom<TOK_RBRACE>;
using lbrack = Atom<TOK_LBRACK>;
using rbrack = Atom<TOK_RBRACK>;
using dot = Atom<TOK_DOT>;
using arrow = Atom<TOK_ARROW>;
using plusplus = Atom<TOK_PLUSPLUS>;
using minusminus = Atom<TOK_MINUSMINUS>;
using comma = Atom<TOK_COMMA>;
using amp = Atom<TOK_AMPERSAND>;
using caret = Atom<TOK_CARET>;
using star = Atom<TOK_STAR>;
using tilde = Atom<TOK_TILDE>;
using bang = Atom<TOK_BANG>;
using plus = Atom<TOK_PLUS>;
using minus = Atom<TOK_MINUS>;
using fslash = Atom<TOK_FSLASH>;
using bslash = Atom<TOK_BSLASH>;
using percent = Atom<TOK_PERCENT>;
using lt = Atom<TOK_LT>;
using gt = Atom<TOK_GT>;
using ltlt = Atom<TOK_LTLT>;
using gtgt = Atom<TOK_GTGT>;
using lteq = Atom<TOK_LTEQ>;
using gteq = Atom<TOK_GTEQ>;
using eqeq = Atom<TOK_EQEQ>;
using bangeq = Atom<TOK_BANGEQ>;
using pipe = Atom<TOK_PIPE>;
using eq = Atom<TOK_EQ>;
using stareq = Atom<TOK_STAREQ>;
using fslasheq = Atom<TOK_FSLASHEQ>;
using percenteq = Atom<TOK_PERCENTEQ>;
using pluseq = Atom<TOK_PLUSEQ>;
using minuseq = Atom<TOK_MINUSEQ>;
using ltlteq = Atom<TOK_LTLTEQ>;
using gtgteq = Atom<TOK_GTGTEQ>;
using ampeq = Atom<TOK_AMPEQ>;
using careteq = Atom<TOK_CARETEQ>;
using pipeeq = Atom<TOK_PIPEEQ>;
using ampamp = Atom<TOK_AMPAMP>;
using pipepipe = Atom<TOK_PIPEPIPE>;
using quest = Atom<TOK_QUEST>;
using colon = Atom<TOK_COLON>;

// Fake
using type_name = Atom<TOK_TYPENAME>;

// no trailing comma
template<typename item>
using comma_separated_list = Seq<item, Any<Seq<comma, item>>>;

// trailing comma ok
template<typename item>
using comma_separated_list2 = Seq<Any<Seq<item, comma>>, Opt<item>>;















#if 0
// (6.5.16)
using assignment_operator = Oneof<
  eq, stareq, fslasheq, percenteq, pluseq, minuseq, ltlteq, gtgteq, ampeq, careteq, pipeeq
>;


// (6.5.3)
using unary_operator = Oneof<amp, star, plus, minus, tilde, bang>;


// (6.5.2)
/*
using postfix_expression = Oneof<
  primary_expression,
  Seq<postfix_expression, lbrack, expression, rbrack>,
  Seq<postfix_expression, lparen, Opt<argument_expression_list>, rparen>,
  Seq<postfix_expression, dot, identifier>,
  Seq<postfix_expression, arrow, identifier>,
  Seq<postfix_expression, plusplus>,
  Seq<postfix_expression, minusminus>,
  Seq<lparen, type_name, rparen, lbrace, initializer_list, rbrace>,
  Seq<lparen, type_name, rparen, lbrace, initializer_list, comma, rbrace>
>;
*/

using postfix_expression = Oneof<
  primary_expression,
  Seq<expression, lbrack, expression, rbrack>,
  Seq<expression, dot, identifier>,
  Seq<expression, arrow, identifier>,
  Seq<expression, plusplus>,
  Seq<expression, minusminus>
>;

using unary_expression = Oneof<
  postfix_expression,
  Seq<plusplus, unary_expression>,
  Seq<minusminus, unary_expression>,
  Seq<unary_operator, cast_expression>,
  Seq<Atom<TOK_SIZEOF>, unary_expression>,
  Seq<Atom<TOK_SIZEOF>, lparen, type_name, rparen>
>;

using cast_expression = Oneof<
  unary_expression,
  Seq<lparen, type_name, rparen, cast_expression>
>;

using multiplicative_expression = Oneof<
  cast_expression,
  Seq<multiplicative_expression, star,    cast_expression>,
  Seq<multiplicative_expression, fslash,  cast_expression>,
  Seq<multiplicative_expression, percent, cast_expression>
>;

using additive_expression = Oneof<
  multiplicative_expression,
  Seq<additive_expression, plus, multiplicative_expression>,
  Seq<additive_expression, minus, multiplicative_expression>
>;

using shift_expression = Oneof<
  additive_expression,
  Seq<shift_expression, ltlt, additive_expression>,
  Seq<shift_expression, gtgt, additive_expression>
>;

using relational_expression = Oneof<
  shift_expression,
  Seq<relational_expression, lt, shift_expression>,
  Seq<relational_expression, gt, shift_expression>,
  Seq<relational_expression, lteq, shift_expression>,
  Seq<relational_expression, gteq, shift_expression>
>;

using equality_expression = Oneof<
  relational_expression,
  Seq<equality_expression, eqeq, relational_expression>,
  Seq<equality_expression, bangeq, relational_expression>
>;

using AND_expression = Oneof<
  equality_expression,
  Seq<AND_expression, amp, equality_expression>
>;

using exclusive_OR_expression = Oneof<
  AND_expression,
  Seq<exclusive_OR_expression, caret, AND_expression>
>;

using inclusive_OR_expression = Oneof<
  exclusive_OR_expression,
  Seq<inclusive_OR_expression, pipe, exclusive_OR_expression>
>;

using logical_AND_expression = Oneof<
  inclusive_OR_expression,
  Seq<logical_AND_expression, ampamp, inclusive_OR_expression>
>;

using logical_OR_expression = Oneof<
  logical_AND_expression,
  Seq<logical_OR_expression, pipepipe, logical_AND_expression>
>;

using conditional_expression = Oneof<
  logical_OR_expression,
  Seq<logical_OR_expression, quest, expression, colon, conditional_expression>
>;

using assignment_expression = Oneof<
  conditional_expression,
  Seq<unary_expression, assignment_operator, assignment_expression>
>;

const token* match_expression(const token* a, const token* b, void* ctx) {
  using expression = Oneof<
    assignment_expression,
    Seq<expression, comma, assignment_expression>
  >;
  return expression::match(a, b, ctx);
}

using constant_expression = conditional_expression;

#endif

/*
//------------------------------------------------------------------------------

(6.7) declaration:
  declaration-specifiers init-declarator-listopt ;

(6.7) declaration-specifiers:
  storage-class-specifier declaration-specifiersopt
  type-specifier declaration-specifiersopt
  type-qualifier declaration-specifiersopt
  function-specifier declaration-specifiersopt

(6.7) init-declarator-list:
  init-declarator
  init-declarator-list , init-declarator

(6.7) init-declarator:
  declarator
  declarator = initializer

(6.7.1) storage-class-specifier:
  typedef
  extern
  static
  auto
  register

//------------------------------------------------------------------------------

(6.7.2) type-specifier:
  void
  char
  short
  int
  long
  float
  double
  signed
  unsigned
  _Bool
  _Complex
  struct-or-union-specifier ∗
  enum-specifier
  typedef-name

(6.7.2.1) struct-or-union-specifier:
  struct-or-union identifieropt { struct-declaration-list }
  struct-or-union identifier

(6.7.2.1) struct-or-union:
  struct
  union
(6.7.2.1) struct-declaration-list:
  struct-declaration
  struct-declaration-list struct-declaration

(6.7.2.1) struct-declaration:
  specifier-qualifier-list struct-declarator-list ;

(6.7.2.1) specifier-qualifier-list:
  type-specifier specifier-qualifier-listopt
  type-qualifier specifier-qualifier-listopt

(6.7.2.1) struct-declarator-list:
  struct-declarator
  struct-declarator-list , struct-declarator

(6.7.2.1) struct-declarator:
  declarator
  declaratoropt : constant_expression

//------------------------------------------------------------------------------

(6.7.2.2) enum-specifier:
  enum identifieropt { enumerator-list }
  enum identifieropt { enumerator-list , }
  enum identifier

(6.7.2.2) enumerator-list:
  enumerator
  enumerator-list , enumerator

(6.7.2.2) enumerator:
  enumeration-constant
  enumeration-constant = constant_expression


//------------------------------------------------------------------------------

(6.7.3) type-qualifier:
  const
  restrict
  volatile

(6.7.4) function-specifier:
  inline

(6.7.5) declarator:
  pointeropt direct-declarator

(6.7.5) direct-declarator:
  identifier
  ( declarator )
  direct-declarator [ type-qualifier-listopt assignment_expressionopt ]
  direct-declarator [ static type-qualifier-listopt assignment_expression ]
  direct-declarator [ type-qualifier-list static assignment_expression ]
  direct-declarator [ type-qualifier-listopt * ]
  direct-declarator ( parameter-type-list )
  direct-declarator ( identifier-listopt )

(6.7.5) pointer:
  * type-qualifier-listopt
  * type-qualifier-listopt pointer

(6.7.5) type-qualifier-list:
  type-qualifier
  type-qualifier-list type-qualifier

(6.7.5) parameter-type-list:
  parameter-list
  parameter-list , ...

(6.7.5) parameter-list:
  parameter-declaration
  parameter-list , parameter-declaration

(6.7.5) parameter-declaration:
  declaration-specifiers declarator
  declaration-specifiers abstract-declaratoropt

(6.7.5) identifier-list:
  identifier
  identifier-list , identifier

(6.7.6) type-name:
  specifier-qualifier-list abstract-declaratoropt

(6.7.6) abstract-declarator:
  pointer
  pointeropt direct-abstract-declarator

(6.7.6) direct-abstract-declarator:
  ( abstract-declarator )
  direct-abstract-declaratoropt [ type-qualifier-listopt
  assignment_expressionopt ]
  direct-abstract-declaratoropt [ static type-qualifier-listopt
  assignment_expression ]
  direct-abstract-declaratoropt [ type-qualifier-list static
  assignment_expression ]
  direct-abstract-declaratoropt [*]
  direct-abstract-declaratoropt ( parameter-type-listopt )

(6.7.7) typedef-name:
  identifier

(6.7.8) initializer:
  assignment_expression
  { initializer-list }
  { initializer-list , }

(6.7.8) initializer-list:
  designationopt initializer
  initializer-list , designationopt initializer

(6.7.8) designation:
  designator-list =

(6.7.8) designator-list:
  designator
  designator-list designator

(6.7.8) designator:
  [ constant_expression ]
  . identifier

(6.8) statement:
  labeled-statement
  compound-statement
  expression-statement
  selection-statement
  iteration-statement
  jump-statement

(6.8.1) labeled-statement:
  identifier : statement
  case constant_expression : statement
  default : statement

(6.8.2) compound-statement:
  { block-item-listopt }

(6.8.2) block-item-list:
  block-item
  block-item-list block-item

(6.8.2) block-item:
  declaration
  statement

(6.8.3) expression-statement:
  expressionopt ;

(6.8.4) selection-statement:
  if ( expression ) statement
  if ( expression ) statement else statement
  switch ( expression ) statement

(6.8.5) iteration-statement:
  while ( expression ) statement
  do statement while ( expression ) ;
  for ( expressionopt ; expressionopt ; expressionopt ) statement
  for ( declaration expressionopt ; expressionopt ) statement

(6.8.6) jump-statement:
  goto identifier ;
  continue ;
  break ;
  return expressionopt ;

(6.9) translation-unit:
  external-declaration
  translation-unit external-declaration

(6.9) external-declaration:
  function-definition
  declaration

(6.9.1) function-definition:
  declaration-specifiers declarator declaration-listopt compound-statement

(6.9.1) declaration-list:
  declaration
  declaration-list declaration
*/
