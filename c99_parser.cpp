#include "Matcheroni.h"

#include "c_lexer.h"
#include "Lexemes.h"
#include "Node.h"
#include "NodeTypes.h"
#include "Tokens.h"

#include <assert.h>
#include <filesystem>
#include <memory.h>
#include <stack>
#include <stdio.h>
#include <vector>

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

const Token* token;
const Token* token_eof;

Node* node_stack[256] = {0};
size_t node_top = 0;

Node* parse_compound_statement();
Node* parse_declaration_list(NodeType type, const char ldelim, const char spacer, const char rdelim);
Node* parse_decltype();
Node* parse_enum_declaration();
Node* parse_expression_list(NodeType type, const char ldelim, const char spacer, const char rdelim);
Node* parse_expression(const char rdelim);
Node* parse_function_call();
Node* parse_identifier();
Node* parse_initializer_list();
Node* parse_parameter_list();
Node* parse_parenthesized_expression();
Node* parse_specifiers();
Node* parse_statement();

//----------------------------------------

const char* find_matching_delim(const char* a, const char* b) {
  char ldelim = *a++;

  char rdelim;
  if (ldelim == '<')  rdelim = '>';
  if (ldelim == '{')  rdelim = '}';
  if (ldelim == '[')  rdelim = ']';
  if (ldelim == '"')  rdelim = '"';
  if (ldelim == '\'') rdelim = '\'';

  while(a && *a && a < b) {
    if (*a == rdelim) return a;

    if (*a == '<' || *a == '{' || *a == '[' || *a == '"' || *a == '\'') {
      a = find_matching_delim(a, b);
      if (!a) return nullptr;
      a++;
    }
    else if (ldelim == '"' && a[0] == '\\' && a[1] == '"') {
      a += 2;
    }
    else if (ldelim == '\'' && a[0] == '\\' && a[1] == '\'') {
      a += 2;
    }
    else {
      a++;
    }
  }

  return nullptr;
}

//----------------------------------------

const Token* find_matching_delim(const Token* a, const Token* b) {
  char ldelim = *a->lex->span_a;
  a++;

  char rdelim;
  if (ldelim == '<')  rdelim = '>';
  if (ldelim == '(')  rdelim = ')';
  if (ldelim == '{')  rdelim = '}';
  if (ldelim == '[')  rdelim = ']';

  while(a && a < b) {
    if (a->is_punct(rdelim)) return a;

    if (a->is_punct('<') ||
        a->is_punct('(') ||
        a->is_punct('{') ||
        a->is_punct('[')) {
      a = find_matching_delim(a, b);
      if (!a) return nullptr;
    }
    a++;
  }

  return nullptr;
}

//----------------------------------------

void skip_lexemes(int count) {
  token = token + count;
}

void skip_punct(const char punct) {
  assert (token->is_punct());
  token++;
}

void skip_opt_punct(const char punct) {
  if (token->is_punct(punct)) token++;
}

void skip_identifier(const char* identifier = nullptr) {
  assert(token->is_identifier(identifier));
  token++;
}

//----------------------------------------

const Token* take_lexemes(NodeType type, int count) {
  node_stack[node_top++] = new Node(type, token, token + count);
  token = token + count;
  return token;
}

Node* take_punct() {
  assert (token->is_punct());
  //return take_lexemes(NODE_PUNCT, 1);
  if (take_lexemes(NODE_PUNCT, 1)) {
    return node_stack[--node_top];
  }
  else {
    return nullptr;
  }
}

Node* take_punct(char punct) {
  assert (token->is_punct(punct));
  //return take_lexemes(NODE_PUNCT, 1);
  if (take_lexemes(NODE_PUNCT, 1)) {
    return node_stack[--node_top];
  }
  else {
    return nullptr;
  }
}

Node* take_opt_punct(char punct) {
  if (token->is_punct(punct)) {
    //return take_lexemes(NODE_PUNCT, 1);
    if (take_lexemes(NODE_PUNCT, 1)) {
      return node_stack[--node_top];
    }
    else {
      return nullptr;
    }
  }
  else {
    return nullptr;
  }
}

Node* take_identifier(const char* identifier = nullptr) {
  assert(token->is_identifier(identifier));
  //return take_lexemes(NODE_IDENTIFIER, 1);
  if (take_lexemes(NODE_IDENTIFIER, 1)) {
    return node_stack[--node_top];
  }
  else {
    return nullptr;
  }
}

Node* take_constant() {
  assert(token->is_constant());
  //return take_lexemes(NODE_CONSTANT, 1);
  if (take_lexemes(NODE_CONSTANT, 1)) {
    return node_stack[--node_top];
  }
  else {
    return nullptr;
  }
}

//----------------------------------------

const Token* parse_access_specifier(const Token* a, const Token* b) {
  using pattern = Seq<
    Oneof<AtomLit<"public">, AtomLit<"private">>,
    Atom<':'>
  >;

  if (auto end = pattern::match(a, b)) {
    node_stack[node_top++] = new Node(NODE_ACCESS_SPECIFIER, a, end);
    return end;
  }
  else {
    return nullptr;
  }
}

//----------------------------------------

Node* parse_declaration(const char rdelim) {
  auto result = new Node(NODE_INVALID);

  bool is_constructor = false;
  bool has_type = false;
  bool has_params = false;
  bool has_body = false;
  bool has_init = false;

  result->append(parse_specifiers());

  if (token[0].is_identifier("enum")) {
    return parse_enum_declaration();
  }


  if (token[0].is_identifier() && token[1].is_punct('(')) {
    is_constructor = true;
    has_type = false;
    result->append(parse_identifier());
  }
  else {
    // Need a better way to handle this
    auto n1 = parse_decltype();

    if (token[0].is_punct('=')) {
      has_type = false;
      n1->node_type = NODE_IDENTIFIER;
      result->append(n1);
    }
    else {
      has_type = true;
      result->append(n1);
      result->append(parse_specifiers());
      result->append(parse_identifier());
    }

    if (token[0].is_punct(':')) {
      result->append(take_punct(':'));
      result->append(parse_expression('{'));
    }
  }

  while (token[0].is_punct('[')) {
    result->append(take_punct('['));
    result->append(parse_expression(']'));
    result->append(take_punct(']'));
  }

  if (token[0].is_punct('(')) {
    has_params = true;
    result->append(parse_parameter_list());

    //result->append(parse_specifiers());
    if (is_constructor) {
      result->append(parse_initializer_list());
    }

    // grab that const after the param list
    result->append(parse_specifiers());
  }

  if (token[0].is_punct('{')) {
    has_body = true;
    result->append(parse_compound_statement());
  }

  if (token[0].is_punct('=')) {
    has_init = true;
    result->append(take_punct('='));
    result->append(parse_expression(rdelim));
  }

  if (!has_type) {
    result->node_type = NODE_INFIX_EXPRESSION;
  }
  else if (is_constructor) {
    result->node_type = NODE_CONSTRUCTOR;
  }
  else if (has_params && has_body) {
    result->node_type = NODE_FUNCTION_DEFINITION;
  }
  else if (has_params) {
    result->node_type = NODE_FUNCTION_DECLARATION;
  }
  else {
    assert(!has_body);
    result->node_type = NODE_DECLARATION;
  }

  return result;
}

//----------------------------------------

Node* parse_field_declaration_list() {
  auto result = new NodeList(NODE_FIELD_DECLARATION_LIST);
  result->ldelim = take_punct('{');
  result->append(result->ldelim);

  while(token < token_eof && !token->is_punct('}')) {

    /*
    if (auto child = parse_access_specifier()) {
      result->append(child);
    }
    */
    if (auto end = parse_access_specifier(token, token_eof)) {
      result->append(node_stack[--node_top]);
      token = end;
    }

    else if (auto child = parse_declaration(';')) {
      result->append(child);
      result->items.push_back(child);
      if (token[0].is_punct(';')) {
        result->append(take_punct(';'));
      }
    }
  }

  result->rdelim = take_punct('}');
  result->append(result->rdelim);
  return result;
}

//----------------------------------------

Node* parse_class_specifier() {

  using decl = Seq<AtomLit<"class">, Atom<LEX_IDENTIFIER>, Atom<';'>>;

  if (auto end = decl::match(token, token_eof)) {
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

Node* parse_struct_specifier() {

  using decl = Seq<AtomLit<"struct">, Atom<LEX_IDENTIFIER>, Atom<';'>>;

  if (auto end = decl::match(token, token_eof)) {
    return new ClassDeclaration(
      take_identifier("struct"),
      take_identifier(),
      take_punct(';')
    );
  }

  return new ClassDefinition(
    take_identifier("struct"),
    take_identifier(),
    parse_field_declaration_list(),
    take_punct(';')
  );
}

Node* parse_namespace_specifier() {

  using decl = Seq<AtomLit<"namespace">, Atom<LEX_IDENTIFIER>, Atom<';'>>;

  if (auto end = decl::match(token, token_eof)) {
    return new ClassDeclaration(
      take_identifier("namespace"),
      take_identifier(),
      take_punct(';')
    );
  }

  return new ClassDefinition(
    take_identifier("namespace"),
    take_identifier(),
    parse_field_declaration_list(),
    take_punct(';')
  );
}

//----------------------------------------

Node* parse_compound_statement() {
  if (!token->is_punct('{')) return nullptr;

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
    if (token->is_punct('*')) {
      specifiers.push_back(take_punct('*'));
      match = true;
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
#if 0
Node* parse_declaration(Node* declaration_specifiers, const char rdelim) {
  // not following the spec here right now

  if (token->is_punct('=')) {
    // Var declaration with init
    auto _declop   = take_punct();
    auto _declinit = parse_expression(rdelim);

    assert(_declop);
    assert(_declinit);

    _declop->field = "op";
    _declinit->field = "init";
    //if (_semi) _semi->field = "semi";

    return new Declaration(
      declaration_specifiers,
      _decltype,
      _declname,
      _declop,
      _declinit
    );
  }
  else if (token->is_punct('(')) {
    // Function declaration or definition

    auto params = parse_parameter_list();

    if (auto body = parse_compound_statement()) {
      auto result = new Node(NODE_FUNCTION_DEFINITION);
      result->append(_decltype);
      result->append(_declname);
      result->append(params);
      result->append(body);
      return result;
    }
    else {
      auto result = new Node(NODE_FUNCTION_DECLARATION);
      result->append(_decltype);
      result->append(_declname);
      result->append(params);
      result->append(take_punct(';'));
      return result;
    }
  }
  else {
    // Var declaration without init
    return new Declaration(declaration_specifiers, _decltype, _declname /*, _semi*/);
  }
}
#endif

//----------------------------------------

Node* parse_decltype() {
  if (token[0].type != TOK_IDENTIFIER) return nullptr;

  Node* type = nullptr;

  // FIXME do some proper type parsing here
  if (token[1].is_punct('*')) {
    type = new Node(NODE_DECLTYPE, &token[0], &token[2]);
    token += 2;
  }
  else {
    type = new Node(NODE_DECLTYPE, &token[0], &token[1]);
    token += 1;
  }

  if (token[0].is_punct('<')) {
    auto result = new Node(NODE_TEMPLATED_TYPE, nullptr, nullptr);
    result->append(type);
    result->append(parse_expression_list(NODE_ARGUMENT_LIST, '<', ',', '>'));

    if (token[0].is_punct(':') && token[1].is_punct(':')) {
      auto result2 = new Node(NODE_SCOPED_TYPE);
      result2->append(result);

      //result2->append(take_lexemes(NODE_OPERATOR, 2));
      if (take_lexemes(NODE_OPERATOR, 2)) {
        result2->append(node_stack[--node_top]);
      }
      else {
      }

      result2->append(parse_decltype());
      return result2;
    }


    return result;
  }
  else {
    return type;
  }
}

//----------------------------------------

Node* parse_expression_prefix() {
  auto span_a = token->lex->span_a;
  auto span_b = token_eof->lex->span_a;

  if (auto end = match_prefix_op(span_a, span_b)) {
    auto match_lex_a = token;
    auto match_lex_b = token;
    while(match_lex_b->lex->span_a < end) match_lex_b++;
    auto result = new Node(NODE_OPERATOR, match_lex_a, match_lex_b);
    token = match_lex_b;
    return result;
  }

  if (token[0].is_punct('(') &&
      token[1].is_identifier() &&
      token[2].is_punct(')')) {
    auto result = new Node(NODE_TYPECAST);
    result->append(take_punct('('));
    result->append(take_identifier());
    result->append(take_punct(')'));
    return result;
  }

  if (token[0].is_identifier("sizeof")) {
    auto result = new Node(NODE_OPERATOR);
    result->append(take_identifier("sizeof"));
    return result;
  }

  return nullptr;
}

//----------------------------------------
// suffix:
//    [ expression ]
//    ( )
//    ( expression )
//    . identifier
//    -> identifier
//    ++
//    --


Node* parse_expression_suffix() {
  if (token[0].is_punct('[')) {
    auto result = new Node(NODE_ARRAY_SUFFIX);
    result->append(take_punct('['));
    result->append(parse_expression(']'));
    result->append(take_punct(']'));
    return result;
  }

  if (token[0].is_punct('(') && token[1].is_punct(')')) {
    auto result = new Node(NODE_PARAMETER_LIST);
    result->append(take_punct('('));
    result->append(take_punct(')'));
    return result;
  }

  if (token[0].is_punct('(')) {
    auto result = new Node(NODE_PARAMETER_LIST);
    result->append(take_punct('('));
    result->append(parse_expression(')'));
    result->append(take_punct(')'));
    return result;
  }

  if (token[0].is_punct('+') && token[1].is_punct('+')) {
    //return take_lexemes(NODE_OPERATOR, 2);
    if (take_lexemes(NODE_OPERATOR, 2)) {
      return node_stack[--node_top];
    }
    else {
      return nullptr;
    }
  }

  if (token[0].is_punct('-') && token[1].is_punct('-')) {
    //return take_lexemes(NODE_OPERATOR, 2);
    if (take_lexemes(NODE_OPERATOR, 2)) {
      return node_stack[--node_top];
    }
    else {
      return nullptr;
    }
  }

  return nullptr;
}

//----------------------------------------

const Token* parse_assignment_op(Token* a, Token* b) {
  return nullptr;
}

Node* parse_assignment_op() {
  auto span_a = token->lex->span_a;
  auto span_b = token_eof->lex->span_a;

  auto end = match_assign_op(span_a, span_b);
  if (!end) return nullptr;

  auto match_lex_a = token;
  auto match_lex_b = token;
  while(match_lex_b->lex->span_a < end) match_lex_b++;
  auto result = new Node(NODE_OPERATOR, match_lex_a, match_lex_b);
  token = match_lex_b;
  return result;
}

//----------------------------------------

const Token* parse_infix_op() {
  auto span_a = token->lex->span_a;
  auto span_b = token_eof->lex->span_a;

  auto end = match_infix_op(span_a, span_b);
  if (!end) return nullptr;

  auto match_lex_a = token;
  auto match_lex_b = token;
  while(match_lex_b->lex->span_a < end) match_lex_b++;
  auto result = new Node(NODE_OPERATOR, match_lex_a, match_lex_b);
  token = match_lex_b;
  node_stack[node_top++] = result;
  return token;
}

//----------------------------------------

Node* parse_expression_rhs(Node* lhs, const char rdelim) {
  if (token->is_eof())         return lhs;
  if (token->is_punct(')'))    return lhs;
  if (token->is_punct(';'))    return lhs;
  if (token->is_punct(','))    return lhs;
  if (token->is_punct(rdelim)) return lhs;

  if (auto op = parse_expression_suffix()) {
    auto result = new Node(NODE_POSTFIX_EXPRESSION);
    result->append(lhs);
    result->append(op);
    return parse_expression_rhs(result, rdelim);
  }

  if (auto end = parse_infix_op()) {
    auto op = node_stack[--node_top];

    auto result = new Node(NODE_INFIX_EXPRESSION);
    auto rhs = parse_expression(rdelim);
    result->append(lhs);
    result->append(op);
    result->append(rhs);
    return parse_expression_rhs(result, rdelim);
  }

  if (token[0].is_punct('[')) {
    auto result = new Node(NODE_ARRAY_EXPRESSION);
    result->append(lhs);
    result->append(take_punct('['));
    result->append(parse_expression(']'));
    result->append(take_punct(']'));
    return parse_expression_rhs(result, rdelim);
  }

  // Nothing found to continue the expression with?
  assert(false);
  return lhs;
}

//----------------------------------------

Node* parse_expression_lhs(const char rdelim) {

  // Dirty hackkkkk - explicitly recognize templated function calls as
  // expression atoms
  if (token[0].is_identifier() &&
      token[1].is_punct('<') &&
      (token[2].is_identifier() || token[2].is_constant()) &&
      token[3].is_punct('>')) {
    auto func = take_identifier();
    auto params1 = parse_expression_list(NODE_TEMPLATE_PARAMETER_LIST, '<', ',', '>');
    auto params2 = parse_expression_list(NODE_ARGUMENT_LIST, '(', ',', ')');

    auto result = new Node(NODE_CALL_EXPRESSION, nullptr, nullptr);
    result->append(func);
    result->append(params1);
    result->append(params2);
    return result;
  }

  if (auto op = parse_expression_prefix()) {
    auto result = new Node(NODE_PREFIX_EXPRESSION);
    result->append(op);
    result->append(parse_expression_lhs(rdelim));
    return result;
  }

  if (token->is_punct('(')) {
    return parse_parenthesized_expression();
  }

  if (token->is_constant()) {
    return take_constant();
  }

  if (token[0].is_identifier() && token[1].is_punct('(')) {
    return parse_function_call();
  }

  if (token[0].is_identifier()) {
    return take_identifier();
  }

  assert(false);
  return nullptr;
}

//----------------------------------------

Node* parse_expression(const char rdelim) {

  // FIXME there are probably other expression terminators?
  if (token->is_eof())      return nullptr;
  if (token->is_punct(')')) return nullptr;
  if (token->is_punct(';')) return nullptr;
  if (token->is_punct(',')) return nullptr;
  if (token->is_punct(rdelim)) return nullptr;

  Node* lhs = parse_expression_lhs(rdelim);
  if (!lhs) return nullptr;

  return parse_expression_rhs(lhs, rdelim);
}

//----------------------------------------

Node* parse_external_declaration() {
  if (token->is_eof()) {
    return nullptr;
  }

  if (token->is_identifier("namespace")) {
    return parse_namespace_specifier();
  }

  if (token->is_identifier("class")) {
    return parse_class_specifier();
  }

  if (token->is_identifier("struct")) {
    return parse_struct_specifier();
  }

  if (auto decl = parse_declaration(';')) {
    decl->append(take_punct(';'));
    return decl;
  }

  return nullptr;
}

//----------------------------------------

Node* parse_enum_declaration() {
  if (!token[0].is_identifier("enum")) return nullptr;

  Node* result = new Node(NODE_ENUM_DECLARATION);

  skip_identifier("enum");

  if (token[0].is_identifier("class")) {
    skip_identifier("class");
  }

  if (token[0].is_identifier()) {
    result->append(take_identifier());
  }

  if (token[0].is_punct(':')) {
    skip_punct(':');
    result->append(parse_decltype());
  }

  if (token[0].is_punct('{')) {
    result->append(parse_expression_list(NODE_ENUMERATOR_LIST, '{', ',', '}'));
  }

  // this is the weird enum {} blah;
  if (token[0].is_identifier()) {
    auto result2 = new Node(NODE_DECLARATION);
    result2->append(result);
    result2->append(take_identifier());
    return result2;
  }

  return result;
}

//----------------------------------------

Node* parse_function_call() {
  if (!token[0].is_identifier() || !token[1].is_punct('(')) return nullptr;

  auto func = take_identifier();
  auto params = parse_expression_list(NODE_ARGUMENT_LIST, '(', ',', ')');

  auto result = new Node(NODE_CALL_EXPRESSION, nullptr, nullptr);
  result->append(func);
  result->append(params);
  return result;
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

Node* parse_return_statement() {
  auto ret = take_identifier();
  auto val = parse_expression(';');
  auto semi = take_punct(';');
  auto result = new Node(NODE_RETURN_STATEMENT, nullptr, nullptr);
  result->append(ret);
  result->append(val);
  result->append(semi);
  return result;
}

//----------------------------------------

Node* parse_parameter_list() {
  auto result = new NodeList(NODE_PARAMETER_LIST);

  result->ldelim = take_punct('(');
  result->append(result->ldelim);

  while(!token->is_punct(')')) {
    auto decl = parse_declaration(',');
    assert(decl);
    result->items.push_back(decl);
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
  result->append(parse_expression(')'));
  result->append(take_punct(')'));
  return result;
}

//----------------------------------------

Node* parse_preproc() {
  using pattern = Atom<TOK_PREPROC>;

  if (auto end = pattern::match(token, token_eof)) {
    auto result = new Node(NODE_PREPROC, token, end);
    token = end;
    return result;
  }

  return nullptr;
}

//----------------------------------------

Node* parse_case_statement() {
  if (!token[0].is_case_label()) return nullptr;

  Node* result = new Node(NODE_CASE_STATEMENT);

  if (token[0].is_identifier("case")) {
    result->append(take_identifier());
    result->append(parse_expression(':'));
    result->append(take_punct(':'));
  }
  else if (token[0].is_identifier("default")) {
    result->append(take_identifier());
    result->append(take_punct(':'));
  }
  else {
    assert(false);
    return nullptr;
  }

  while (!token[0].is_case_label() && !token[0].is_punct('}')) {
    result->append(parse_statement());
  }
  return result;
}

//----------------------------------------

Node* parse_switch_statement() {
  if (!token[0].is_identifier("switch")) return nullptr;

  Node* result = new Node(NODE_SWITCH_STATEMENT);

  result->append(take_identifier("switch"));
  result->append(parse_expression_list(NODE_ARGUMENT_LIST, '(', ',', ')'));
  result->append(take_punct('{'));

  while(!token[0].is_punct('}')) {
    result->append(parse_case_statement());
  }

  result->append(take_punct('}'));
  return result;
}

//----------------------------------------

Node* parse_declaration_or_expression(char rdelim) {
  auto result1 = parse_declaration(rdelim);
  if (result1) return result1;
  return parse_expression(rdelim);
}

//----------------------------------------

Node* parse_for_statement() {
  if (!token[0].is_identifier("for")) return nullptr;

  auto result = new Node(NODE_FOR_STATEMENT);

  result->tok_a = token;

  auto old_size = node_top;

  skip_identifier("for");
  skip_punct('(');
  result->append(parse_declaration_or_expression(';'));
  skip_punct(';');
  result->append(parse_expression(';'));
  skip_punct(';');
  result->append(parse_expression(')'));
  skip_punct(')');
  result->append(parse_statement());

  auto new_size = node_top;

  result->tok_b = token;

  return result;
}

//----------------------------------------

Node* parse_statement() {
  if (token[0].is_punct('{')) {
    return parse_compound_statement();
  }

  if (token[0].is_identifier("if")) {
    return parse_if_statement();
  }

  if (token[0].is_identifier("while")) {
    return parse_while_statement();
  }

  if (token[0].is_identifier("for")) {
    return parse_for_statement();
  }

  if (token[0].is_identifier("return")) {
    return parse_return_statement();
  }

  if (token[0].is_identifier("switch")) {
    return parse_switch_statement();
  }

  if (token[0].is_identifier() && token[1].is_identifier()) {
    auto result = new Node(NODE_DECLARATION_STATEMENT, nullptr, nullptr);
    result->append(parse_declaration(';'));
    result->append(take_punct(';'));
    return result;
  }

  // Dirty hack
  if (token[0].is_identifier() &&
      token[1].is_punct('<') &&
      (token[2].is_identifier() || token[2].is_constant()) &&
      token[3].is_punct('>') &&
      token[4].is_identifier()) {
    auto result = new Node(NODE_DECLARATION_STATEMENT, nullptr, nullptr);
    result->append(parse_declaration(';'));
    result->append(take_punct(';'));
    return result;
  }

  // Must be expression statement
  auto exp = parse_expression(';');
  auto semi = take_punct(';');
  auto result = new Node(NODE_EXPRESSION_STATEMENT, nullptr, nullptr);
  result->append(exp);
  result->append(semi);
  return result;
}

//----------------------------------------

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
  result->append(parse_declaration_list(NODE_TEMPLATE_PARAMETER_LIST, '<', ',', '>'));
  result->append(parse_class_specifier());

  result->_keyword = result->child(0);
  result->_params  = result->child(1);
  result->_class   = result->child(2);

  return result;
}

//----------------------------------------

Node* parse_declaration_list(NodeType type, const char ldelim, const char spacer, const char rdelim) {
  if (!token[0].is_punct(ldelim)) return nullptr;

  //auto tok_rdelim = find_matching_delim(token, token_eof);

  auto result = new NodeList(type);

  skip_punct(ldelim);

  while(1) {
    if (token->is_punct(rdelim)) {
      skip_punct(rdelim);
      break;
    }
    else if (token->is_punct(spacer)) {
      skip_punct(spacer);
    }
    else {
      result->append(parse_declaration(rdelim));
    }
  }

  return result;
}

//----------------------------------------

Node* parse_expression_list(NodeType type, const char ldelim, const char spacer, const char rdelim) {
  if (!token[0].is_punct(ldelim)) return nullptr;

  auto result = new NodeList(type);

  skip_punct(ldelim);

  while(1) {
    if (token->is_punct(rdelim)) {
      skip_punct(rdelim);
      break;
    }
    else if (token->is_punct(spacer)) {
      skip_punct(spacer);
    }
    else {
      result->append(parse_expression(rdelim));
    }
  }

  return result;
}

//----------------------------------------

Node* parse_initializer_list() {
  char ldelim = ':';
  char spacer = ',';
  char rdelim = '{'; // we don't consume this unlike parse_expression_list

  if (!token[0].is_punct(ldelim)) return nullptr;

  auto result = new Node(NODE_INITIALIZER_LIST);

  skip_punct(ldelim);

  while(1) {
    if (token->is_punct(rdelim)) {
      return result;
    }

    if (token->is_punct(spacer)) {
      skip_punct(spacer);
    }
    else {
      result->append(parse_expression(rdelim));
    }
  }

  return result;
}

//----------------------------------------

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

Node* parse_translation_unit() {
  auto result = new TranslationUnit();

  while(!token->is_eof()) {

    if (Lit<"template">::match(token->lex->span_a, token_eof->lex->span_a)) {
      result->append(parse_template_decl());
    }
    else if (auto decl = parse_preproc()) {
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
































//==============================================================================

void lex_file(const std::string& path, std::string& text, std::vector<Lexeme>& lexemes, std::vector<Token>& tokens) {
  printf("Lexing %s\n", path.c_str());

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

int test_c99_peg(int argc, char** argv) {
  printf("Parseroni Demo\n");

  using rdit = std::filesystem::recursive_directory_iterator;

  auto time_a = timestamp_ms();
  std::vector<std::string> paths;

  //const char* base_path = "tests";
  //const char* base_path = "mini_tests";
  const char* base_path = argc > 1 ? argv[1] : "tests";


  printf("Parsing source files in %s\n", base_path);
  for (const auto& f : rdit(base_path)) {
    if (!f.is_regular_file()) continue;
    paths.push_back(f.path().native());
  }

  //paths = { "tests/constructor_arg_passing.h" };

  for (const auto& path : paths) {
    std::string text;
    std::vector<Lexeme> lexemes;
    std::vector<Token> tokens;

    lex_file(path, text, lexemes, tokens);

    token = tokens.data();
    token_eof = tokens.data() + tokens.size() - 1;

    auto root = parse_translation_unit();
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
