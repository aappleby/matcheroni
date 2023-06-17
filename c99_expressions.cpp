#include "c99_parser.h"
#include "Node.h"

struct NodeExpression;
struct NodeExpressionParen;
//struct NodeExpressionBraces;
struct NodeExpressionSoup;
struct NodeExpressionSubscript;
struct NodeExpressionCall;
struct NodeExpressionTernary;
struct NodeGccCompoundExpression;

const Token* parse_initializer_list(const Token* a, const Token* b);
const Token* parse_statement_compound(const Token* a, const Token* b);

//------------------------------------------------------------------------------

struct NodeExpressionSoup : public PatternWrapper<NodeExpressionSoup> {
  using pattern = Seq<
    Some<Oneof<
      NodeGccCompoundExpression,
      NodeExpressionCall,
      NodeExpressionParen,
      //NodeExpressionBraces,
      Ref<parse_initializer_list>,
      NodeExpressionSubscript,
      NodeKeyword<"sizeof">,
      NodeOperator<"->*">,
      NodeOperator<"<<=">,
      NodeOperator<"<=>">,
      NodeOperator<">>=">,

      NodeOperator<"--">,
      NodeOperator<"-=">,
      NodeOperator<"->">,
      NodeOperator<"::">,
      NodeOperator<"!=">,
      NodeOperator<".*">,
      NodeOperator<"*=">,
      NodeOperator<"/=">,
      NodeOperator<"&&">,
      NodeOperator<"&=">,
      NodeOperator<"%=">,
      NodeOperator<"^=">,
      NodeOperator<"++">,
      NodeOperator<"+=">,
      NodeOperator<"<<">,
      NodeOperator<"<=">,
      NodeOperator<"==">,
      NodeOperator<">=">,
      NodeOperator<">>">,
      NodeOperator<"|=">,
      NodeOperator<"||">,

      NodeOperator<"-">,
      NodeOperator<"!">,
      NodeOperator<".">,
      NodeOperator<"*">,
      NodeOperator<"/">,
      NodeOperator<"&">,
      NodeOperator<"%">,
      NodeOperator<"^">,
      NodeOperator<"+">,
      NodeOperator<"<">,
      NodeOperator<"=">,
      NodeOperator<">">,
      NodeOperator<"|">,
      NodeOperator<"~">,

      NodeIdentifier,
      NodeConstant
    >>
  >;
};

struct NodeExpressionParen : public NodeMaker<NodeExpressionParen> {
  using pattern = Seq<
    Atom<'('>,
    Opt<comma_separated<NodeExpression>>,
    Atom<')'>
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeExpressionParen>::match(a, b);
    return end;
  }
};

/*
struct NodeExpressionBraces : public NodeMaker<NodeExpressionBraces> {
  using pattern = Seq<
    Atom<'{'>,
    Opt<comma_separated<NodeExpression>>,
    Atom<'}'>
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeExpressionBraces>::match(a, b);
    return end;
  }
};
*/

struct NodeExpressionCall : public NodeMaker<NodeExpressionCall> {
  using pattern = Seq<
    NodeIdentifier,
    NodeExpressionParen
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeExpressionCall>::match(a, b);
    return end;
  }
};

struct NodeExpressionSubscript : public NodeMaker<NodeExpressionSubscript> {
  using pattern = Seq<
    Atom<'['>,
    NodeExpression,
    Atom<']'>
  >;
};

struct NodeGccCompoundExpression : public NodeMaker<NodeGccCompoundExpression> {
  using pattern = Seq<
    Atom<'('>,
    Ref<parse_statement_compound>,
    Atom<')'>
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeGccCompoundExpression>::match(a, b);
    return end;
  }
};

struct NodeExpressionTernary : public PatternWrapper<NodeExpressionTernary> {
  using pattern = Seq<
    NodeExpressionSoup,
    Opt<Seq<
      NodeOperator<"?">,
      NodeExpressionTernary,
      NodeOperator<":">,
      NodeExpressionTernary
    >>
  >;
};

struct NodeExpression : public NodeMaker<NodeExpression> {
  using pattern = NodeExpressionTernary;
};

const Token* parse_expression(const Token* a, const Token* b) {
  auto end = NodeExpression::match(a, b);
  return end;
}

//------------------------------------------------------------------------------
