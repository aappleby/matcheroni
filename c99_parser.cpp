#include "Matcheroni.h"
#include <assert.h>
#include <stdio.h>
#include <memory.h>
#include "c_lexer.h"
#include "Node.h"
#include "NodeTypes.h"

#include "Lexemes.h"
#include "Tokens.h"

#include <stack>
#include <vector>
#include <filesystem>

double timestamp_ms();

using namespace matcheroni;

bool atom_eq(const Token& a, const TokenType& b) {
  return a.type == b;
}

bool atom_eq(const Token& a, const char& b) {
  return a.lex->len() == 1 && (*a.lex->span_a == b);
}

bool atom_lte(const Token& a, const char& b) {
  return a.lex->len() == 1 && (*a.lex->span_a <= b);
}

bool atom_gte(const Token& a, const char& b) {
  return a.lex->len() == 1 && (*a.lex->span_a >= b);
}

template<int N>
bool atom_eq(const Token& a, const matcheroni::StringParam<N>& b) {
  if (a.lex->len() != b.len) return false;
  for (auto i = 0; i < b.len; i++) {
    if (a.lex->span_a[i] != b.value[i]) return false;
  }

  return true;
}

//------------------------------------------------------------------------------

const char* text = R"(
#include <stdio.h>

// This is a comment.

int main(int argc, char** argv) {
  // Another comment
  printf("Hello () World %d %p!\n", 12345, argv);
  return 0;
}

)";

//------------------------------------------------------------------------------

/*
Node* fold_nodes(Node* a, Node* b, NodeType tok) {
  auto parent = a->parent;

  auto n = new Node(tok, a->tok_a, b->tok_b);

  n->parent = nullptr;
  n->prev = a->prev;
  n->next = b->next;
  n->head = a;
  n->tail = b;

  if (n->prev) n->prev->next = n;
  if (n->next) n->next->prev = n;

  if (parent->head == a) parent->head = n;
  if (parent->tail == b) parent->tail = n;

  a->prev = nullptr;
  b->next = nullptr;

  for (auto c = a; c; c = c->next) c->parent = n;

  return n;
}
*/

//------------------------------------------------------------------------------

/*
void fold_delims(Node* a, Node* b, char ldelim, char rdelim, NodeType block_type) {

  std::stack<Node*> ldelims;

  for (Node* cursor = a; cursor; cursor = cursor->next) {
    if (cursor->tok_a->lex->type != LEX_PUNCT) continue;

    if (*cursor->tok_a->lex->span_a == ldelim) {
      ldelims.push(cursor);
    }
    else if (*cursor->tok_a->lex->span_a == rdelim) {
      auto node_ldelim = ldelims.top();
      auto node_rdelim = cursor;
      cursor = fold_nodes(node_ldelim, node_rdelim, block_type);
      ldelims.pop();
    }
  }
}
*/

const char* c_specifiers[] = {
  "const",
  "constexpr",
  "extern",
  "inline",
  "register",
  "restrict",
  "signed",
  "static",
  "thread_local",
  "unsigned",
  "volatile",
  "_Noreturn",
};

const char* c_primitives[] = {
  "auto",
  "bool",
  "char",
  "double",
  "float",
  "int",
  "long",
  "short",
  "void",
};

const char* c_control[] = {
  "break",
  "case",
  "continue",
  "default",
  "do",
  "else",
  "for",
  "goto",
  "if",
  "return",
  "switch",
  "while",
};

const char* c_constants[] = {
  "false",
  "nullptr",
  "true",
};

const char* c_intrinsics[] = {
  "alignas",
  "alignof",
  "sizeof",
  "static_assert",
  "typeof",
  "typeof_unqual",
};

const char* c_structs[] = {
  "enum",
  "struct",
  "typedef",
  "union",
};

//------------------------------------------------------------------------------

struct Parser {
  const Token* token;
  const Token* token_eof;

  //----------------------------------------

  Node* take_lexeme() {
    auto result = new Node(tok_to_node(token->type), token, token + 1);
    token = token + 1;
    return result;
  }

  Node* take_punct() {
    assert (token->is_punct());
    return take_lexeme();
  }

  void skip_punct(char punct) {
    assert (token->is_punct(punct));
    token++;
  }

  Node* take_punct(char punct) {
    assert (token->is_punct(punct));
    return take_lexeme();
  }

  Node* take_opt_punct(char punct) {
    if (token->is_punct(punct)) {
      return take_lexeme();
    }
    else {
      return nullptr;
    }
  }

  Node* take_identifier(const char* identifier = nullptr) {
    assert(token->is_identifier(identifier));
    return take_lexeme();
  }

  Node* take_constant() {
    assert(token->is_constant());
    return take_lexeme();
  }

  //----------------------------------------

  Node* parse_access_specifier() {
    using pattern = Seq<
      Oneof<AtomLit<"public">, AtomLit<"private">>,
      Atom<':'>
    >;

    if (auto end = pattern::match(token, token_eof, nullptr)) {
      Node* result = new Node(NODE_ACCESS_SPECIFIER, token, end);
      token = end;
      return result;
    }
    return nullptr;
  }

  //----------------------------------------
  /*
    class_specifier: $ => seq(
      'class',
      $._class_declaration
    ),
    // When used in a trailing return type, these specifiers can now occur immediately before
    // a compound statement. This introduces a shift/reduce conflict that needs to be resolved
    // with an associativity.
    _class_declaration: $ => prec.right(seq(
      optional($.ms_declspec_modifier),
      repeat($.attribute_declaration),
      choice(
        field('name', $._class_name),
        seq(
          optional(field('name', $._class_name)),
          optional($.virtual_specifier),
          optional($.base_class_clause),
          field('body', $.field_declaration_list)
        )
      )
    )),
  */

  template<typename P>
  const Token* match() {
    return P::match(token, token_eof, nullptr);
  }

  Node* parse_class_specifier() {

    using decl = Seq<AtomLit<"class">, Atom<LEX_IDENTIFIER>, Atom<';'>>;

    if (auto end = match<decl>()) {
      return new ClassDeclaration(
        take_identifier("class"),
        take_identifier(),
        take_punct(';')
      );
    }

    return new ClassDefinition(
      take_identifier("class"),
      take_identifier(),
      parse_field_declaration_list(),
      take_punct(';')
    );
  }

  //----------------------------------------

  Node* parse_compound_statement() {
    auto result = new CompoundStatement();

    result->ldelim = take_punct('{');
    result->append(result->ldelim);

    while(1) {
      if (token->is_punct('}')) break;
      auto statement = parse_statement();
      assert(statement);
      result->statements.push_back(statement);
      result->append(statement);
    }

    result->rdelim = take_punct('}');
    result->append(result->rdelim);

    return result;
  }

  //----------------------------------------

  Node* parse_specifiers() {
    const char* keywords[] = {
      "extern",
      "static",
      "register",
      "inline",
      "thread_local",
      "const",
      "volatile",
      "restrict",
      "__restrict__",
      "_Atomic",
      "_Noreturn",
      "mutable",
      "constexpr",
      "constinit",
      "consteval",
      "virtual",
      "explicit",
    };
    auto keyword_count = sizeof(keywords)/sizeof(keywords[0]);

    std::vector<Node*> specifiers;

    while(token < token_eof) {
      bool match = false;
      for (auto i = 0; i < keyword_count; i++) {
        if (token->is_identifier(keywords[i])) {
          match = true;
          specifiers.push_back(take_identifier());
          break;
        }
      }
      if (!match) break;
    }

    if (specifiers.size()) {
      auto result = new Node(NODE_SPECIFIER_LIST);
      for (auto n : specifiers) result->append(n);
      return result;
    }
    else {
      return nullptr;
    }
  }

  //----------------------------------------

  /*
    struct Constructor {
      vector<Node*> specifiers;
      Node* name;
      Node* params;
      Node* initializer_list;
      Node* body;
    };

    field_initializer_list: $ => seq(
      ':',
      commaSep1($.field_initializer)
    ),

    constructor_or_destructor_definition: $ => seq(
      repeat(constructor_specifiers),
      identifier,
      parameter_list,
      optional(field_initializer_list),
      compound_statement
    ),

  */

  Node* parse_constructor(Node* _specs) {

    // Constructor
    Node* _decl   = take_identifier();
    Node* _params = parse_parameter_list();
    Node* _body   = nullptr;

    if (token->is_punct('{')) {
      _body = parse_compound_statement();
    }
    else if (token->is_punct(';')) {
      skip_punct(';');
    }
    else {
      assert(false);
    }

    return new Constructor(_specs, _decl, _params, _body);
  }

  //----------------------------------------
  /*
  (6.7) declaration:
    declaration-specifiers init-declarator-listopt ;
    attribute-specifier-sequence declaration-specifiers init-declarator-list ;
    static_assert-declaration
    attribute-declaration
  */
  Node* parse_declaration(Node* declaration_specifiers) {
    // not following the spec here right now

    auto _decltype = parse_decltype();
    auto _declname = parse_identifier();

    _decltype->field = "type";
    _declname->field = "name";

    if (token->is_punct('=')) {
      // Var declaration with init
      auto _declop   = take_punct();
      auto _declinit = parse_expression();
      auto _semi     = take_opt_punct(';');

      assert(_declop);
      assert(_declinit);

      _declop->field = "op";
      _declinit->field = "init";
      if (_semi) _semi->field = "semi";

      return new Declaration(
        declaration_specifiers,
        _decltype,
        _declname,
        _declop,
        _declinit,
        _semi);
    }
    else if (token->is_punct('(')) {
      // Function declaration or definition
      assert(false);
      return nullptr;
    }
    else {
      // Var declaration without init
      auto _semi = take_opt_punct(';');
      if (_semi) _semi->field = "semi";
      return new Declaration(declaration_specifiers, _decltype, _declname, _semi);
    }
  }

  //----------------------------------------

  Node* parse_specifier() {
    Node* result = nullptr;

    for (auto c : c_specifiers) {
      if (token->is_identifier(c)) {
        auto result = new Node(NODE_SPECIFIER, token, token+1);
        token = token + 1;
        return result;
      }
    }

    return nullptr;
  }

  //----------------------------------------

  Node* parse_specifier_list() {

    std::vector<Node*> specs;

    while(1) {
      if (auto child = parse_specifier()) {
        specs.push_back(child);
      }
      else {
        break;
      }
    }

    if (specs.size()) {
      auto result = new Node(NODE_SPECIFIER_LIST, nullptr, nullptr);
      for (auto s : specs) result->append(s);
      return result;
    }

    return nullptr;
  }

  //----------------------------------------

  Node* parse_decltype() {
    // FIXME placeholder
    if (token->type != TOK_IDENTIFIER) return nullptr;
    auto result = new Node(NODE_DECLTYPE, token, token + 1);
    token = token + 1;
    return result;
  }

  //----------------------------------------

  Node* parse_expression_atom() {
    if (token->is_punct('(')) {
      return parse_parenthesized_expression();
    }

    if (token->is_constant()) {
      return take_constant();
    }

    if (token[0].is_identifier()) {
      if (token[1].is_punct('(')) {
        return parse_function_call();
      }
      else {
        return take_identifier();
      }
    }

    assert(false);
    return nullptr;
  }

  //----------------------------------------

  Node* parse_infix_op() {
    auto span_a = token->lex->span_a;
    auto span_b = token_eof->lex->span_a;

    auto end = match_infix_op(span_a, span_b, nullptr);
    if (!end) return nullptr;

    auto match_lex_a = token;
    auto match_lex_b = token;
    while(match_lex_b->lex->span_a < end) match_lex_b++;
    auto result = new Node(NODE_OPERATOR, match_lex_a, match_lex_b);
    token = match_lex_b;
    return result;
  }

  Node* parse_prefix_op() {
    auto span_a = token->lex->span_a;
    auto span_b = token_eof->lex->span_a;

    auto end = match_prefix_op(span_a, span_b, nullptr);
    if (!end) return nullptr;

    auto match_lex_a = token;
    auto match_lex_b = token;
    while(match_lex_b->lex->span_a < end) match_lex_b++;
    auto result = new Node(NODE_OPERATOR, match_lex_a, match_lex_b);
    token = match_lex_b;
    return result;
  }

  Node* parse_postfix_op() {
    auto span_a = token->lex->span_a;
    auto span_b = token_eof->lex->span_a;

    auto end = match_postfix_op(span_a, span_b, nullptr);
    if (!end) return nullptr;

    auto match_lex_a = token;
    auto match_lex_b = token;
    while(match_lex_b->lex->span_a < end) match_lex_b++;
    auto result = new Node(NODE_OPERATOR, match_lex_a, match_lex_b);
    token = match_lex_b;
    return result;
  }

  //----------------------------------------

  Node* parse_expression() {

    // FIXME there are probably other expression terminators?
    if (token->is_eof())      return nullptr;
    if (token->is_punct(')')) return nullptr;
    if (token->is_punct(';')) return nullptr;
    if (token->is_punct(',')) return nullptr;

    Node* lhs = nullptr;

    if (auto op = parse_prefix_op()) {
      lhs = new Node(NODE_PREFIX_EXPRESSION);
      lhs->append(op);
      lhs->append(parse_expression_atom());
    }
    else {
      lhs = parse_expression_atom();
    }

    if (!lhs) return nullptr;

    if (auto op = parse_postfix_op()) {
      auto result = new Node(NODE_POSTFIX_EXPRESSION);
      result->append(lhs);
      result->append(op);
      lhs = result;
    }

    if (auto op = parse_infix_op()) {
      auto result = new Node(NODE_INFIX_EXPRESSION);
      auto rhs = parse_expression();
      result->append(lhs);
      result->append(op);
      result->append(rhs);
      return result;
    }

    // this doesn't seem right...
    return lhs;
  }

  //----------------------------------------
  /*
  (6.9) external-declaration:
    function-definition
    declaration
  */

  Node* parse_external_declaration() {
    if (token->is_eof()) return nullptr;

    if (token->is_identifier("class")) {
      return parse_class_specifier();
    }

    auto specifiers = parse_specifier_list();

    if (auto func = parse_function_definition(specifiers)) {
      return func;
    }
    else if (auto decl = parse_declaration(specifiers)) {
      return decl;
    }
    else {
      return nullptr;
    }
  }

  //----------------------------------------
  /*
    _field_declaration_list_item: ($, original) => choice(
      original,
      $.template_declaration,
      alias($.inline_method_definition, $.function_definition),
      alias($.constructor_or_destructor_definition, $.function_definition),
      alias($.constructor_or_destructor_declaration, $.declaration),
      alias($.operator_cast_definition, $.function_definition),
      alias($.operator_cast_declaration, $.declaration),
      $.friend_declaration,
      seq($.access_specifier, ':'),
      $.alias_declaration,
      $.using_declaration,
      $.type_definition,
      $.static_assert_declaration
    ),
  */

  Node* parse_field_declaration() {

    auto specifiers = parse_specifier_list();

    if (token[0].is_identifier() && token[1].is_punct('(')) {
      return parse_constructor(specifiers);
    }
    else if (token[0].is_identifier() && token[1].is_identifier() && token[2].is_punct('(')) {
      return parse_function_definition(specifiers);
    }
    else {
      return parse_declaration(specifiers);
    }
  }

  //----------------------------------------

  Node* parse_field_declaration_list() {
    auto result = new FieldDeclarationList();
    result->ldelim = take_punct('{');
    result->append(result->ldelim);
    while(token < token_eof) {
      if (token->is_punct('}')) {
        break;
      }
      else if (auto child = parse_access_specifier()) {
        result->append(child);
      }
      else if (auto child = parse_field_declaration()) {
        result->append(child);
        result->decls.push_back(child);
      }
    }
    result->rdelim = take_punct('}');
    result->append(result->rdelim);
    return result;
  }

  //----------------------------------------

  Node* parse_function_call() {
    assert(false);
    return nullptr;
  }

  //----------------------------------------
  /*
  (6.7.6) direct-declarator:
    identifier attribute-specifier-sequenceopt
    ( declarator )
    array-declarator attribute-specifier-sequenceopt
    function-declarator attribute-specifier-sequenceopt

  (6.7.6) function-declarator:
    direct-declarator ( parameter-type-listopt )
  */

  Node* parse_function_declarator() {
    return nullptr;
  }

  //----------------------------------------
  /*
  using function_definition = Seq<
    Opt<attribute_specifier_sequence>,
    declaration_specifiers,
    declarator,
    function_body
  >;
  (6.9.1) function-definition:
  attribute-specifier-sequenceopt declaration-specifiers declarator function-body
  (6.9.1) function-body:
  compound-statement
  */

  Node* parse_function_definition(Node* declaration_specifiers) {

    if (token[0].is_identifier() && token[1].is_identifier() && token[2].is_punct('(')) {
      auto type = take_identifier();
      auto name = take_identifier();
      auto params = parse_parameter_list();

      if (token[0].is_punct(';')) {
        assert(false);
        return nullptr;
      }
      else if (token[0].is_punct('{')) {
        auto body = parse_compound_statement();

        auto result = new Node(NODE_FUNCTION_DEFINITION, nullptr, nullptr);
        result->append(type);
        result->append(name);
        result->append(params);
        result->append(body);
        return result;
      }
      else {
        assert(false);
        return nullptr;
      }

    }
    else {
      assert(false);
      return nullptr;
    }
  }

  //----------------------------------------

  Node* parse_identifier() {
    if (token->type != TOK_IDENTIFIER) return nullptr;
    auto result = new Node(NODE_IDENTIFIER, token, token + 1);
    token = token + 1;
    return result;
  }

  //----------------------------------------

  Node* parse_if_statement() {
    auto result = new Node(NODE_IF_STATEMENT);
    result->append(take_identifier("if"));
    result->append(parse_parenthesized_expression());
    result->append(parse_statement());
    if (token[0].is_identifier("else")) {
      result->append(take_identifier("else"));
      result->append(parse_statement());
    }
    return result;
  }

  Node* parse_while_statement() {
    auto result = new Node(NODE_WHILE_STATEMENT);
    result->append(take_identifier("while"));
    result->append(parse_parenthesized_expression());
    result->append(parse_statement());
    return result;
  }

  //----------------------------------------

  Node* parse_parameter_list() {
    auto result = new ParameterList();

    result->ldelim = take_punct('(');
    result->append(result->ldelim);

    while(!token->is_punct(')')) {
      auto specs = parse_specifier_list();
      auto decl = parse_declaration(specs);
      assert(decl);
      result->decls.push_back(decl);
      result->append(decl);
      if (token->is_punct(',')) {
        result->append(take_punct(','));
      }
    }

    result->rdelim = take_punct(')');
    result->append(result->rdelim);

    return result;
  }

  //----------------------------------------

  Node* parse_parenthesized_expression() {
    auto result = new Node(NODE_PARENTHESIZED_EXPRESSION);
    result->append(take_punct('('));
    result->append(parse_expression());
    result->append(take_punct(')'));
    return result;
  }

  //----------------------------------------

  Node* parse_preproc() {
    using pattern = Atom<TOK_PREPROC>;

    if (auto end = pattern::match(token, token_eof, nullptr)) {
      auto result = new Node(NODE_PREPROC, token, end);
      token = end;
      return result;
    }

    return nullptr;
  }

  //----------------------------------------

  Node* parse_statement() {
    if (token[0].is_punct('{')) {
      return parse_compound_statement();
    }

    if (token[0].is_identifier("if")) {
      return parse_if_statement();
    }

    auto specs = parse_specifier_list();

    if (token[0].is_identifier() && token[1].is_identifier()) {
      if (token[2].is_punct('=') || token[2].is_punct(';')) {
        return parse_declaration(specs);
      }
      else {
        assert(false);
        return nullptr;
      }
    }
    else {
      assert(false);
      return nullptr;
    }
  }

  //----------------------------------------
  /*
  storage_class_specifier: _ => choice(
    'extern',
    'static',
    'auto',
    'register',
    'inline',
  ),
  */

  Node* parse_storage_class_specifier() {
    const char* keywords[] = {
      "extern",
      "static",
      "auto",
      "register",
      "inline",
    };
    auto keyword_count = sizeof(keywords)/sizeof(keywords[0]);
    for (auto i = 0; i < keyword_count; i++) {
      if (token->is_identifier(keywords[i])) {
        return take_identifier();
      }
    }

    return nullptr;
  }

  //----------------------------------------

  Node* parse_template_decl() {
    auto result = new TemplateDeclaration();

    result->append(take_identifier("template"));
    result->append(parse_template_parameter_list());
    result->append(parse_class_specifier());

    result->_keyword = result->child(0);
    result->_params  = result->child(1);
    result->_class   = result->child(2);

    return result;
  }

  //----------------------------------------

  Node* parse_template_parameter_list() {

    Node* result = new Node(NODE_TEMPLATE_PARAMETER_LIST, nullptr, nullptr);

    skip_punct('<');

    while(1) {
      auto specs = parse_specifier_list();
      result->append(parse_declaration(specs));
      if (token->is_punct('>')) {
        skip_punct('>');
        break;
      }
      else if (token->is_punct(',')) {
        skip_punct(',');
      }
      else {
        assert(false);
      }
    }

    return result;
  }

  //----------------------------------------
  /*
  (6.9) translation-unit:
    external-declaration
    translation-unit external-declaration

  translation_unit: $ => repeat($._top_level_item),

  _empty_declaration: $ => seq(
    $._type_specifier,
    ';',
  ),

  attributed_statement: $ => seq(
    repeat1($.attribute_declaration),
    $._statement,
  ),

  _top_level_item: $ => choice(
    $.function_definition,
    $.linkage_specification,
    $.declaration,
    $._statement,
    $.attributed_statement,
    $.type_definition,
    $._empty_declaration,
  ),
  */

  Node* parse_translation_unit() {
    auto result = new TranslationUnit();

    while(!token->is_eof()) {

      if (Lit<"template">::match(token->lex->span_a, token_eof->lex->span_a, nullptr)) {
        result->append(parse_template_decl());
      }
      if (auto decl = parse_preproc()) {
        result->append(decl);
      }
      else if (auto decl = parse_external_declaration()) {
        result->append(decl);
      }
      else {
        assert(false);
        break;
      }
    }

    return result;
  }

  //----------------------------------------
  /*
  type_definition: $ => seq(
    'typedef',
    repeat($.type_qualifier),
    field('type', $._type_specifier),
    commaSep1(field('declarator', $._type_declarator)),
    ';',
  ),
  */

  Node* parse_type_definition() {
    return nullptr;
  }

  //----------------------------------------

  Node* parse_typeof_specifier() {
    Node* result = nullptr;
    // FIXME
    //using typeof_specifier_argument = Oneof<expression, type_name>;

    using typeof_specifier_argument = Atom<LEX_IDENTIFIER>;

    using typeof_specifier = Oneof<
      Seq< AtomLit<"typeof">, Atom<'('>, typeof_specifier_argument, Atom<')'> >,
      Seq< AtomLit<"typeof_unqual">, Atom<'('>, typeof_specifier_argument, Atom<')'> >
    >;

    if (auto end = typeof_specifier::match(token, token_eof, nullptr)) {
      result = new Node(NODE_TYPEOF_SPECIFIER, token, end);
      token = end;
    }

    return result;
  }

  //----------------------------------------

  Node* parse_typedef_name() {
    Node* result = nullptr;
    using typedef_name = Atom<LEX_IDENTIFIER>;

    if (auto end = typedef_name::match(token, token_eof, nullptr)) {
      result = new Node(NODE_TYPEDEF_NAME, token, end);
      token = end;
    }

    return result;
  }

  //----------------------------------------

  Node* parse_type_specifier(Lexeme* a, Lexeme* b) {
    return nullptr;
  }

  //----------------------------------------
  /*
  (6.7.3) type-qualifier:
    const
    restrict
    volatile
    _Atomic

  type_qualifier: _ => choice(
    'const',
    'volatile',
    'restrict',
    '__restrict__',
    '_Atomic',
    '_Noreturn',
  ),
  */

  Node* parse_type_qualifier() {
    const char* keywords[] = {
      "const",
      "volatile",
      "restrict",
      "__restrict__",
      "_Atomic",
      "_Noreturn",
    };
    auto keyword_count = sizeof(keywords)/sizeof(keywords[0]);
    for (auto i = 0; i < keyword_count; i++) {
      if (token->is_identifier(keywords[i])) {
        return take_identifier();
      }
    }

    assert(false);
    return nullptr;
  }

  //----------------------------------------

  Node* parse_type_specifier_qualifier() {
    assert(false);
    return nullptr;
  }


};

//------------------------------------------------------------------------------

void lex_file(const std::string& path, int size, std::string& text, std::vector<Lexeme>& lexemes, std::vector<Token>& tokens) {
  printf("Lexing %s\n", path.c_str());
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
    if (lex.type == LEX_EOF) {
      for (int i = 0; i < 10; i++) lexemes.push_back(lex);
      break;
    }
    else {
      lexemes.push_back(lex);
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

int test_c99_peg() {
  printf("Hello World\n");

  using rdit = std::filesystem::recursive_directory_iterator;

  //const char* base_path = "tests";
  const char* base_path = "mini_tests";

  printf("Parsing source files in %s\n", base_path);
  auto time_a = timestamp_ms();

  for (const auto& f : rdit(base_path)) {
    if (!f.is_regular_file()) continue;

    auto& path = f.path().native();
    auto size = f.file_size();
    std::string text;
    std::vector<Lexeme> lexemes;
    std::vector<Token> tokens;

    lex_file(path, size, text, lexemes, tokens);

    //dump_lexemes(path, size, text, lexemes);

    Parser p;
    p.token = tokens.data();
    p.token_eof = tokens.data() + tokens.size() - 1;

    auto root = p.parse_translation_unit();
    printf("\n");
    root->dump_tree();
    printf("\n");
    delete root;

  }

  auto time_b = timestamp_ms();

  printf("Parsing took %f msec\n", time_b - time_a);

  return 0;
}

//------------------------------------------------------------------------------
