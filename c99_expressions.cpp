#include "c99_parser.h"
#include "Node.h"

struct NodeExpressionParen;
struct NodeExpressionSoup;
struct NodeExpressionSubscript;

//------------------------------------------------------------------------------

struct NodeExpressionSoup : public NodeMaker<NodeExpressionSoup> {
  using pattern = Seq<
    Some<Oneof<
      NodeConstant,
      NodeIdentifier,
      NodeExpressionParen,
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
      NodeOperator<":">,
      NodeOperator<"!">,
      NodeOperator<"?">,
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
      NodeOperator<"~">
    >>
  >;
};

struct NodeExpressionParen : public NodeMaker<NodeExpressionParen> {
  using pattern = Seq<
    Atom<'('>,
    NodeExpressionSoup,
    Atom<')'>
  >;
};

struct NodeExpressionSubscript : public NodeMaker<NodeExpressionSubscript> {
  using pattern = Seq<
    Atom<'['>,
    NodeExpressionSoup,
    Atom<']'>
  >;
};

const Token* parse_expression(const Token* a, const Token* b) {
  auto end = NodeExpressionSoup::match(a, b);
  return end;
}

//------------------------------------------------------------------------------
