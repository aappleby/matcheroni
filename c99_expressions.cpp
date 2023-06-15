#include "c99_parser.h"
#include "Node.h"

struct NodeExpressionParen;
struct NodeExpressionSoup;
struct NodeExpressionSubscript;
struct NodeExpressionCall;

//------------------------------------------------------------------------------

struct NodeExpressionSoup : public NodeMaker<NodeExpressionSoup> {
  using pattern = Seq<
    Some<Oneof<
      NodeExpressionCall,
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
    Opt<comma_separated<NodeExpressionSoup>>,
    Atom<')'>
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeExpressionParen>::match(a, b);
    return end;
  }
};

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
    NodeExpressionSoup,
    Atom<']'>
  >;
};

const Token* parse_expression(const Token* a, const Token* b) {
  auto end = NodeExpressionSoup::match(a, b);
  return end;
}

//------------------------------------------------------------------------------
