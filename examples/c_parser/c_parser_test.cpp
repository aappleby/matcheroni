// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include <filesystem>
#include <stdio.h>
#include <assert.h>

#include "matcheroni/Utilities.hpp"

#include "examples/c_lexer/CLexer.hpp"
#include "examples/c_parser/CContext.hpp"
#include "examples/c_parser/CNode.hpp"
#include "examples/c_parser/c_parse_nodes.hpp"

using namespace matcheroni;

bool no_ws_streq(const std::string& a, const std::string& b) {
  auto ca = a.c_str();
  auto cb = b.c_str();

  while(1) {
    while(isspace(*ca)) ca++;
    while(isspace(*cb)) cb++;
    if (auto d = *ca - *cb) return false;
    if (*ca == 0) return true;
    ca++;
    cb++;
  }
}

void parse_and_dump(matcher_function<CContext, CToken> parse, std::string source, std::string expected) {
  CLexer lexer;
  CContext context;
  auto text_span = utils::to_span(source);
  auto lex_ok = lexer.lex(text_span);
  TokSpan tok_span = utils::to_span(lexer.tokens);

  //auto parse_ok = context.parse(text_span, tok_span);

  std::vector<CToken> tokens;

  for (auto t : lexer.tokens) {
    if (!t.is_gap()) tokens.push_back(t);
  }

  // Skip over BOF, stop before EOF
  auto tok_a = tokens.data() + 1;
  auto tok_b = tokens.data() + tokens.size() - 1;
  TokSpan body(tok_a, tok_b);

  auto tail = parse(context, body);

  std::string dump;
  context.debug_dump(dump);

  if (!expected.empty()) {
    assert(no_ws_streq(dump, expected));
  }
  else {
    printf("%s\n", dump.c_str());
  }
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("c_parser_test\n");

  std::string source;
  std::string result;

  {
    // Dunno how to do this one yet...
    using pattern = NodeAbstractDeclarator;
    //parse_and_dump(pattern::match, source, result);
  }

  {
    using pattern = Capture<"NodeAccessSpecifier", NodeAccessSpecifier, CNode>;
    source = "public:\n";
    result = "[NodeAccessSpecifier:`public:`]";
    parse_and_dump(pattern::match, source, result);
  }

  {
    using pattern = Capture<"NodeAlignas", NodeAlignas, CNode>;
    source = "_Alignas(int)\n";
    result = "[NodeAlignas:[value:[specifier:[builtin:[base_type:`int`]]][abstract_decl:``]]]";
    parse_and_dump(pattern::match, source, result);

    source = "_Alignas(8)\n";
    result = "[NodeAlignas:[value:[int:`8`]]]";
    parse_and_dump(pattern::match, source, result);
  }

  {
    using pattern = Capture<"NodeBitSuffix", NodeBitSuffix, CNode>;
    source = ":123";
    result = "[NodeBitSuffix:[expression:[unit:[constant:[int:`123`]]]]]";
    parse_and_dump(pattern::match, source, result);
  }

  {
    using pattern = Capture<"NodeAttribute", NodeAttribute, CNode>;
    source = "__attribute__((foobar))";
    result = "[NodeAttribute:[expression:[unit:[identifier:`foobar`]]]]";
    parse_and_dump(pattern::match, source, result);

    source = "__attribute((foobar))";
    result = "[NodeAttribute:[expression:[unit:[identifier:`foobar`]]]]";
    parse_and_dump(pattern::match, source, result);
  }

  {
    using pattern = Capture<"NodeDeclspec", NodeDeclspec, CNode>;
    source = "__declspec(foobar)";
    result = "[NodeDeclspec:[identifier:`foobar`]]";
    parse_and_dump(pattern::match, source, result);
  }

  {
    using pattern = Capture<"NodeEllipsis", NodeEllipsis, CNode>;
    source = "...";
    result = "[NodeEllipsis:`...`]";
    parse_and_dump(pattern::match, source, result);
  }

  {
    using pattern = Capture<"NodeQualifier", NodeQualifier, CNode>;
    source = "__volatile__";
    result = "[NodeQualifier:`__volatile__`]";
    parse_and_dump(pattern::match, source, result);
  }

  {
    using pattern = Capture<"NodeStatementBreak", NodeStatementBreak, CNode>;
    source = "break;";
    result = "[NodeStatementBreak:`break;`]";
    parse_and_dump(pattern::match, source, result);
  }

  {
    using pattern = Capture<"NodeStatementCase", NodeStatementCase, CNode>;
    source = "case 7: break;";
    result = "[NodeStatementCase:[condition:[unit:[constant:[int:`7`]]]][body:[break:`break;`]]]";
    parse_and_dump(pattern::match, source, result);
  }

  {
    using pattern = Capture<"NodeEnumerator", NodeEnumerator, CNode>;
    source = "RED = 7";
    result = "[NodeEnumerator:[name:`RED`][value:[unit:[constant:[int:`7`]]]]]";
    parse_and_dump(pattern::match, source, result);
  }

  {
    using pattern = Capture<"NodeEnumerators", NodeEnumerators, CNode>;
    source = "{ RED = 1, BLUE = 2, GREEN = 3 }";
    result = R"(
    [NodeEnumerators:
      [enumerator:[name:`RED`  ][value:[unit:[constant:[int:`1`]]]]]
      [enumerator:[name:`BLUE` ][value:[unit:[constant:[int:`2`]]]]]
      [enumerator:[name:`GREEN`][value:[unit:[constant:[int:`3`]]]]]
    ])";
    parse_and_dump(pattern::match, source, result);
  }
  /*
  {
    using pattern = Capture<"", , CNode>;
    source = "";
    result = "";
    parse_and_dump(pattern::match, source, result);
  }
  */

  printf("c_parser_test done\n");
  return 0;
}

//------------------------------------------------------------------------------

/*
struct NodeArraySuffix
struct NodeAsmQualifiers
struct NodeAsmRef
struct NodeAsmRefs
struct NodeAsmSuffix
struct NodeAtom
struct NodeAtomicType
struct NodeBase
struct NodeBinaryOp
struct NodeBuiltinType
struct NodeClass
struct NodeClassType
struct NodeClassTypeAdder
struct NodeConstant
struct NodeContext
struct NodeDeclaration
struct NodeDeclarator
struct NodeDeclaratorList
struct NodeDesignation
struct NodeDispenser
struct NodeEnum
struct NodeEnumerator
struct NodeEnumerators
struct NodeEnumType
struct NodeEnumTypeAdder
struct NodeExpression
struct NodeExpressionAlignof
struct NodeExpressionBinary
struct NodeExpressionBraces
struct NodeExpressionGccCompound
struct NodeExpressionOffsetof
struct NodeExpressionParen
struct NodeExpressionPrefix
struct NodeExpressionSizeof
struct NodeExpressionSuffix
struct NodeExpressionTernary
struct NodeField
struct NodeFieldList
struct NodeFunctionDefinition
struct NodeFunctionIdentifier
struct NodeIdentifier
struct NodeInitializer
struct NodeInitializerList
struct NodeIterator
struct NodeKeyword
struct NodeLiteral
struct NodeModifier
struct NodeNamespace
struct NodeParam
struct NodeParamList
struct NodeParenType
struct NodePointer
struct NodePrefixCast
struct NodePrefixKeyword
struct NodePrefixOp
struct NodePreproc
struct NodeSpecifier
struct NodeStatement
struct NodeStatementAsm
struct NodeStatementCompound
struct NodeStatementContinue
struct NodeStatementDefault
struct NodeStatementDoWhile
struct NodeStatementElse
struct NodeStatementFor
struct NodeStatementGoto
struct NodeStatementIf
struct NodeStatementLabel
struct NodeStatementReturn
struct NodeStatementSwitch
struct NodeStatementWhile
struct NodeSuffixBraces
struct NodeSuffixInitializerList
struct NodeSuffixOp
struct NodeSuffixParen
struct NodeSuffixSubscript
struct NodeTemplate
struct NodeTemplateArgs
struct NodeTemplateParams
struct NodeTernaryOp
struct NodeTranslationUnit
struct NodeTypeDecl
struct NodeTypedef
struct NodeTypedefType
struct NodeTypeName
struct NodeUnion
struct NodeUnionType
struct NodeUnionTypeAdder
*/
