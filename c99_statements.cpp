#include "c99_parser.h"
#include "Node.h"

const Token* parse_function   (const Token* a, const Token* b);
const Token* parse_struct     (const Token* a, const Token* b);
const Token* parse_class      (const Token* a, const Token* b);
const Token* parse_union      (const Token* a, const Token* b);
const Token* parse_enum       (const Token* a, const Token* b);
const Token* parse_qualifiers (const Token* a, const Token* b);
//------------------------------------------------------------------------------

struct NodeStatementCompound : public NodeMaker<NodeStatementCompound> {
  using pattern = Seq<
    Atom<'{'>,
    Any<Ref<parse_statement>>,
    Atom<'}'>
  >;
};

const Token* parse_statement_compound(const Token* a, const Token* b) {
  return NodeStatementCompound::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeStatementDeclaration : public NodeMaker<NodeStatementDeclaration> {
  using pattern = Ref<parse_declaration>;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeStatementDeclaration>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeStatementExpression : public NodeMaker<NodeStatementExpression> {
  using pattern = comma_separated<Ref<parse_expression>>;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeStatementExpression>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeStatementFor : public NodeMaker<NodeStatementFor> {
  using pattern = Seq<
    Keyword<"for">,
    Atom<'('>,
    Opt<comma_separated<Oneof<
      Ref<parse_expression>,
      Ref<parse_declaration>
    >>>,
    Atom<';'>,
    Opt<comma_separated<Ref<parse_expression>>>,
    Atom<';'>,
    Opt<comma_separated<Ref<parse_expression>>>,
    Atom<')'>,
    Ref<parse_statement>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementElse : public NodeMaker<NodeStatementElse> {
  using pattern =
  Seq<
    Keyword<"else">,
    Ref<parse_statement>
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeStatementElse>::match(a, b);
    return end;
  }

};

struct NodeStatementIf : public NodeMaker<NodeStatementIf> {
  using pattern = Seq<
    Keyword<"if">,
    Seq<
      Atom<'('>,
      comma_separated<Ref<parse_expression>>,
      Atom<')'>
    >,
    Ref<parse_statement>,
    Opt<NodeStatementElse>
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeStatementIf>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeStatementReturn : public NodeMaker<NodeStatementReturn> {
  using pattern = Seq<
    Keyword<"return">,
    Ref<parse_expression>
  >;
};


//------------------------------------------------------------------------------

struct NodeStatementCase : public NodeMaker<NodeStatementCase> {
  using pattern = Seq<
    Keyword<"case">,
    Ref<parse_expression>,
    Atom<':'>,
    Any<Seq<
      Not<Keyword<"case">>,
      Not<Keyword<"default">>,
      Ref<parse_statement>
    >>
  >;
};

struct NodeStatementDefault : public NodeMaker<NodeStatementDefault> {
  using pattern = Seq<
    Keyword<"default">,
    Atom<':'>,
    Any<Seq<
      Not<Keyword<"case">>,
      Not<Keyword<"default">>,
      Ref<parse_statement>
    >>
  >;
};

struct NodeStatementSwitch : public NodeMaker<NodeStatementSwitch> {
  using pattern = Seq<
    Keyword<"switch">,
    Ref<parse_expression>,
    Atom<'{'>,
    Any<Oneof<
      NodeStatementCase,
      NodeStatementDefault
    >>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementWhile : public NodeMaker<NodeStatementWhile> {
  using pattern = Seq<
    Keyword<"while">,
    Atom<'('>,
    comma_separated<Ref<parse_expression>>,
    Atom<')'>,
    Ref<parse_statement>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementDoWhile : public NodeMaker<NodeStatementDoWhile> {
  using pattern = Seq<
    Keyword<"do">,
    Ref<parse_statement>,
    Keyword<"while">,
    Atom<'('>,
    Ref<parse_expression>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementLabel: public NodeMaker<NodeStatementLabel> {
  using pattern = Seq<
    NodeIdentifier,
    Atom<':'>
  >;
};

//------------------------------------------------------------------------------

struct NodeAsmRef : public NodeMaker<NodeAsmRef> {
  using pattern = Seq<
    NodeString,
    Opt<Seq<
      Atom<'('>,
      Ref<parse_expression>,
      Atom<')'>
    >>
  >;
};

struct NodeAsmRefs : public NodeMaker<NodeAsmRefs> {
  using pattern = comma_separated<NodeAsmRef>;
};

//------------------------------------------------------------------------------

struct NodeAsmQualifiers : public NodeMaker<NodeAsmQualifiers> {
  using pattern =
  Some<Oneof<
    NodeKeyword<"volatile">,
    NodeKeyword<"__volatile">,
    NodeKeyword<"__volatile__">,
    NodeKeyword<"inline">,
    NodeKeyword<"goto">
  >>;
};

//------------------------------------------------------------------------------

struct NodeStatementAsm : public NodeMaker<NodeStatementAsm> {
  using pattern = Seq<
    Oneof<
      Keyword<"asm">,
      Keyword<"__asm">,
      Keyword<"__asm__">
    >,
    //Ref<parse_qualifiers>,
    Opt<NodeAsmQualifiers>,
    Atom<'('>,
    NodeString, // assembly code
    Atom<':'>,
    Opt<NodeAsmRefs>, // output operands
    Opt<Seq<
      Atom<':'>,
      Opt<NodeAsmRefs>, // input operands
      Opt<Seq<
        Atom<':'>,
        Opt<NodeAsmRefs>, // clobbers
        Opt<Seq<
          Atom<':'>,
          Opt<comma_separated<NodeIdentifier>> // GotoLabels
        >>
      >>
    >>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementGoto : public NodeMaker<NodeStatementGoto> {
  using pattern = Seq<
    Keyword<"goto">,
    NodeIdentifier
  >;
};

//------------------------------------------------------------------------------

const Token* parse_statement(const Token* a, const Token* b) {
  using pattern_statement = Oneof<
    // All of these have keywords first
    Seq<Ref<parse_class>, Atom<';'>>,
    Seq<Ref<parse_struct>, Atom<';'>>,
    Seq<Ref<parse_union>, Atom<';'>>,
    NodeStatementFor,
    NodeStatementIf,
    Seq<NodeStatementReturn, Atom<';'>>,
    NodeStatementSwitch,
    Seq<NodeStatementDoWhile, Atom<';'>>,
    NodeStatementWhile,
    Seq<NodeStatementAsm, Atom<';'>>,

    // These don't - but they might confuse a keyword with an identifier...
    NodeStatementLabel,
    NodeStatementCompound,
    Ref<parse_function>,
    Seq<NodeStatementDeclaration, Atom<';'>>,
    Seq<NodeStatementExpression, Atom<';'>>,

    // Extra semicolons
    Atom<';'>
  >;

  auto end = pattern_statement::match(a, b);
  return end;
}

//------------------------------------------------------------------------------
