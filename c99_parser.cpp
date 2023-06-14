#include "Matcheroni.h"

#include "Lexemes.h"
#include "Node.h"
#include "NodeTypes.h"
#include "Tokens.h"

#include <filesystem>
#include <memory.h>

double timestamp_ms();

#ifdef MATCHERONI_USE_NAMESPACE
using namespace matcheroni;
#endif


/*
Keyword<"_Bool">,
Keyword<"_Complex">,
Keyword<"__m128">,
Keyword<"__m128d">,
Keyword<"__m128i">
*/

//------------------------------------------------------------------------------
// Predecls

const Token* parse_statement(const Token* a, const Token* b);
const Token* parse_expression(const Token* a, const Token* b);

//------------------------------------------------------------------------------

struct NodeSpecifier : public NodeMaker<NodeSpecifier> {
  using pattern = Oneof<
    Keyword<"extern">,
    Keyword<"static">,
    Keyword<"register">,
    Keyword<"inline">,
    Keyword<"thread_local">,
    Keyword<"const">,
    Keyword<"volatile">,
    Keyword<"restrict">,
    Keyword<"__restrict__">,
    Keyword<"_Atomic">,
    Keyword<"_Noreturn">,
    Keyword<"mutable">,
    Keyword<"constexpr">,
    Keyword<"constinit">,
    Keyword<"consteval">,
    Keyword<"virtual">,
    Keyword<"explicit">
  >;
};

//------------------------------------------------------------------------------

struct NodeSpecifierList : public NodeMaker<NodeSpecifierList> {
  using pattern = Some<NodeSpecifier>;
};

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

struct NodeTemplateArgs : public NodeMaker<NodeTemplateArgs> {
  using pattern = Delimited<'<', '>',
    opt_comma_separated<Ref<parse_expression>>
  >;
};

//------------------------------------------------------------------------------

struct NodeDecltype : public NodeMaker<NodeDecltype> {
  using pattern = Seq<
    Opt<NodeSpecifierList>,
    Opt<Keyword<"struct">>,
    NodeTypeName,
    Opt<NodeSpecifierList>,
    Opt<NodeTemplateArgs>,
    Opt<Seq<
      Atom<':'>,
      Atom<':'>,
      NodeIdentifier
    >>,
    Opt<Atom<'*'>>
  >;
};

//------------------------------------------------------------------------------
// FIXME should probably have a few diffeerent versions instead of all the opts

struct NodeEnum : public NodeMaker<NodeEnum> {
  using pattern = Seq<
    Keyword<"enum">,
    Opt<Keyword<"class">>,
    Opt<NodeIdentifier>,
    Opt<Seq<Atom<':'>, NodeDecltype>>,
    Opt<Seq<
      Atom<'{'>,
      comma_separated<Ref<parse_expression>>,
      Atom<'}'>
    >>,
    Opt<NodeIdentifier>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeArraySuffix : public NodeMaker<NodeArraySuffix> {
  using pattern = Seq<
    Atom<'['>,
    Ref<parse_expression>,
    Atom<']'>
  >;
};

//------------------------------------------------------------------------------

struct NodeBitsizeSuffix : public NodeMaker<NodeBitsizeSuffix> {
  using pattern = Seq<
    Atom<':'>,
    Ref<parse_expression>
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclarationAtom : public NodeMaker<NodeDeclarationAtom> {
  using pattern = Seq<
    NodeIdentifier,
    Opt<NodeBitsizeSuffix>,
    Any<NodeArraySuffix>,
    Opt<Seq<
      Atom<'='>,
      Ref<parse_expression>
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclaration : public NodeMaker<NodeDeclaration> {
  using pattern = Seq<
    NodeDecltype,
    Oneof<
      comma_separated<NodeDeclarationAtom>,
      Some<NodeArraySuffix>,
      Nothing
    >
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeDeclaration>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeStatementDeclaration : public NodeMaker<NodeStatementDeclaration> {
  using pattern = Seq<
    NodeDeclaration,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclList : public NodeMaker<NodeDeclList> {
  using pattern = Seq<
    Atom<'('>,
    opt_comma_separated<NodeDeclaration>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeInitializerList : public NodeMaker<NodeInitializerList> {
  using pattern = Seq<
    Atom<':'>,
    comma_separated<Ref<parse_expression>>,
    And<Atom<'{'>>
  >;
};

struct NodeConstructor : public NodeMaker<NodeConstructor> {
  using pattern = Seq<
    NodeIdentifier,
    NodeDeclList,
    Opt<NodeInitializerList>,
    Oneof<
      NodeStatementCompound,
      Atom<';'>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeFunction : public NodeMaker<NodeFunction> {
  using pattern = Seq<
    NodeDecltype,
    NodeIdentifier,
    NodeDeclList,
    Opt<Keyword<"const">>,
    Oneof<
      NodeStatementCompound,
      Atom<';'>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeAccessSpecifier : public NodeMaker<NodeAccessSpecifier> {
  using pattern = Seq<
    Oneof<
      Keyword<"public">,
      Keyword<"private">
    >,
    Atom<':'>
  >;
};

//------------------------------------------------------------------------------

struct NodeFieldList : public NodeMaker<NodeFieldList> {
  using pattern = Seq<
    Atom<'{'>,
    Any<Oneof<
      NodeAccessSpecifier,
      NodeConstructor,
      NodeEnum,
      NodeFunction,
      NodeStatementDeclaration
    >>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeClass : public NodeMaker<NodeClass> {
  using pattern = Seq<
    Keyword<"class">,
    NodeIdentifier,
    Opt<NodeFieldList>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStruct : public NodeMaker<NodeStruct> {
  using pattern = Seq<
    Keyword<"struct">,
    NodeIdentifier,
    Opt<NodeFieldList>,
    Atom<';'>
  >;
};

//----------------------------------------

struct NodeNamespace : public NodeMaker<NodeNamespace> {
  using pattern = Seq<
    Keyword<"namespace">,
    NodeIdentifier,
    Opt<NodeFieldList>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeTemplateParams : public NodeMaker<NodeTemplateParams> {
  using pattern = Delimited<'<', '>', comma_separated<NodeDeclaration>>;
};





















































//==============================================================================
// EXPRESSION STUFF

struct NodeExpressionParen : public NodeMaker<NodeExpressionParen> {
  using pattern = Seq<
    Atom<'('>,
    Ref<parse_expression>,
    Atom<')'>
  >;
};

struct NodeExpressionSubscript : public NodeMaker<NodeExpressionSubscript> {
  using pattern = Seq<
    Atom<'['>,
    Ref<parse_expression>,
    Atom<']'>
  >;
};

struct NodeExpressionSoup : public NodeMaker<NodeExpressionSoup> {
  using pattern = Seq<
    Any<Oneof<
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

const Token* parse_expression(const Token* a, const Token* b) {

  using pattern = NodeExpressionSoup;

  return pattern::match(a, b);

}









































































//------------------------------------------------------------------------------

struct NodeStatementIf : public NodeMaker<NodeStatementIf> {
  using pattern = Seq<
    Keyword<"if">,
    Seq<
      Atom<'('>,
      Ref<parse_expression>,
      Atom<')'>
    >,
    Ref<parse_statement>,
    Opt<Seq<
      Keyword<"else">,
      Ref<parse_statement>
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementWhile : public NodeMaker<NodeStatementWhile> {
  using pattern = Seq<
    Keyword<"while">,
    Seq<
      Atom<'('>,
      Ref<parse_expression>,
      Atom<')'>
    >,
    Ref<parse_statement>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementReturn : public NodeMaker<NodeStatementReturn> {
  using pattern = Seq<
    Keyword<"return">,
    Ref<parse_expression>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodePreproc : public NodeMaker<NodePreproc> {
  using pattern = Atom<TOK_PREPROC>;
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

//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------

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

struct NodeStatementFor : public NodeMaker<NodeStatementFor> {
  using pattern = Seq<
    Keyword<"for">,
    Atom<'('>,
    Opt<Oneof<
      Ref<parse_expression>,
      NodeDeclaration
    >>,
    Atom<';'>,
    Opt<Ref<parse_expression>>,
    Atom<';'>,
    Opt<Ref<parse_expression>>,
    Atom<')'>,
    Ref<parse_statement>
  >;
};

//----------------------------------------

struct NodeStatementExpression : public NodeMaker<NodeStatementExpression> {
  using pattern = Seq<
    Ref<parse_expression>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

const Token* parse_statement(const Token* a, const Token* b) {
  using pattern_statement = Oneof<
    NodeStatementCompound,
    NodeStatementIf,
    NodeStatementWhile,
    NodeStatementFor,
    NodeStatementReturn,
    NodeStatementSwitch,
    NodeStatementDeclaration,
    NodeStatementExpression
  >;

  return pattern_statement::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeDeclarationTemplate : public NodeMaker<NodeDeclarationTemplate> {
  using pattern = Seq<
    Keyword<"template">,
    NodeTemplateParams,
    NodeClass
  >;
};

//------------------------------------------------------------------------------

/*
 TestPattern 'typedef const int* blep[7];'
 | declaration 'typedef const int* blep[7];'
 |  | NodeKeyword 'typedef'
 |  | typeQualifier 'const'
 |  | NodeTypeName 'int'
 |  | declarator '* blep[7]'
 |  |  | pointer '*'
 |  |  |  | NodeOperator '*'
 |  |  | NodeIdentifier 'blep'
 |  |  | NodeExpressionSoup '7'
 |  |  |  | NodeConstant '7'
 */

template<typename P>
struct StoreTypedef {
  static const Token* match(const Token* a, const Token* b) {
    if (auto end = P::match(a, b)) {
      // Poke through the node on the top of the stack to find identifiers

      if (auto n1 = NodeBase::node_stack.back()) {
        if (auto n2 = n1->child<NodeDeclarationAtom>()) {
          if (auto n3 = n2->child<NodeIdentifier>()) {
            auto l = n3->tok_a->lex;
            auto s = std::string(l->span_a, l->span_b);
            NodeBase::global_types.insert(s);
          }
        }
      }
      return end;
    }
    else {
      return nullptr;
    }
  }
};

struct NodeStatementTypedef : public NodeMaker<NodeStatementTypedef> {
  using pattern = StoreTypedef<
    Seq<
      Keyword<"typedef">,
      NodeDeclaration,
      Atom<';'>
    >
  >;
};

//------------------------------------------------------------------------------

template<typename P>
struct ProgressBar {
  static const Token* match(const Token* a, const Token* b) {
    printf("%.40s\n", a->lex->span_a);
    return P::match(a, b);
  }
};

struct NodeTranslationUnit : public NodeMaker<NodeTranslationUnit> {
  using pattern = Any<
    ProgressBar<
      Oneof<
        NodePreproc,
        NodeNamespace,
        NodeStatementTypedef,
        NodeEnum,
        NodeDeclarationTemplate,
        NodeClass,
        NodeStruct,
        NodeFunction,
        NodeStatementDeclaration
      >
    >
  >;
};

//------------------------------------------------------------------------------







































































//==============================================================================

void lex_file(const std::string& path, std::string& text, std::vector<Lexeme>& lexemes, std::vector<Token>& tokens) {

  auto size = std::filesystem::file_size(path);

  text.resize(size + 1);
  memset(text.data(), 0, size + 1);
  FILE* file = fopen(path.c_str(), "rb");
  if (!file) {
    printf("Could not open %s!\n", path.c_str());
  }
  auto r = fread(text.data(), 1, size, file);

  auto text_a = text.data();
  auto text_b = text_a + size;

  const char* cursor = text_a;
  while(cursor) {
    auto lex = next_lexeme(cursor, text_b);
    lexemes.push_back(lex);
    if (lex.type == LEX_EOF) {
      break;
    }
    cursor = lex.span_b;
  }

  for (auto i = 0; i < lexemes.size(); i++) {
    auto l = &lexemes[i];
    if (!l->is_gap()) {
      tokens.push_back(Token(lex_to_tok(l->type), l));
    }
  }
}

//------------------------------------------------------------------------------

void dump_lexemes(const std::string& path, int size, std::string& text, std::vector<Lexeme>& lexemes) {
  for(auto& l : lexemes) {
    printf("%-15s ", l.str());

    int len = l.span_b - l.span_a;
    if (len > 80) len = 80;

    for (int i = 0; i < len; i++) {
      auto text_a = text.data();
      auto text_b = text_a + size;
      if (l.span_a + i >= text_b) {
        putc('#', stdout);
        continue;
      }

      auto c = l.span_a[i];
      if (c == '\n' || c == '\t' || c == '\r') {
        putc('@', stdout);
      }
      else {
        putc(l.span_a[i], stdout);
      }
    }
    //printf("%-.40s", l.span_a);
    printf("\n");

    if (l.is_eof()) break;
  }
}

//------------------------------------------------------------------------------

const Token* parse_declaration(const Token* a, const Token* b);
const Token* parse_external_declaration(const Token* a, const Token* b);

struct TestPattern : public NodeMaker<TestPattern> {
  using pattern = Some<Ref<parse_external_declaration>>;
};

//------------------------------------------------------------------------------

int oneof_count = 0;

int test_c99_peg(int argc, char** argv) {
  printf("Parseroni Demo\n");


  std::vector<std::string> paths;
  const char* base_path = argc > 1 ? argv[1] : "tests";

  printf("Parsing source files in %s\n", base_path);
  using rdit = std::filesystem::recursive_directory_iterator;
  for (const auto& f : rdit(base_path)) {
    if (!f.is_regular_file()) continue;
    paths.push_back(f.path().native());
  }

  paths = { "tests/scratch.h" };
  //paths = { "mini_tests/csmith.cpp" };

  double lex_accum = 0;
  double parse_accum = 0;

  for (const auto& path : paths) {
    std::string text;
    std::vector<Lexeme> lexemes;
    std::vector<Token> tokens;

    printf("Lexing %s\n", path.c_str());

    lex_accum -= timestamp_ms();
    lex_file(path, text, lexemes, tokens);
    lex_accum += timestamp_ms();

    const Token* token_a = tokens.data();
    const Token* token_b = tokens.data() + tokens.size() - 1;

    printf("Parsing %s\n", path.c_str());

    parse_accum -= timestamp_ms();
    //NodeTranslationUnit::match(token_a, token_b);
    TestPattern::match(token_a, token_b);
    parse_accum += timestamp_ms();

    if (NodeBase::node_stack.top() != 1) {
      printf("Node stack wrong size %ld\n", NodeBase::node_stack._top);
      return -1;
    }

    auto root = NodeBase::node_stack.pop();

    root->dump_tree();

    if (root->tok_a != token_a) {
      printf("\n");
      printf("XXXXXX Root's first token is not token_a!\n");
      printf("XXXXXX Root's first token is not token_a!\n");
      printf("\n");
    }
    else if (root->tok_b != token_b) {
      printf("\n");
      printf("XXXXXX Root's last token is not token_b!\n");
      printf("XXXXXX Root's last token is not token_b!\n");
      printf("\n");
    }
    else {
      printf("Root OK\n");
    }

    delete root;

  }

  printf("Lexing took  %f msec\n", lex_accum);
  printf("Parsing took %f msec\n", parse_accum);

  printf("oneof_count = %d\n", oneof_count);

  return 0;
}

//------------------------------------------------------------------------------
