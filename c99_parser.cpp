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

Node* pop_node()         { return node_stack[--node_top]; }
void  push_node(Node* n) { node_stack[node_top++] = n; }

const Token* parse_compound_statement();
const Token* parse_declaration_list(NodeType type, const char ldelim, const char spacer, const char rdelim);
const Token* parse_decltype();
const Token* parse_enum_declaration();
const Token* parse_expression_list(NodeType type, const char ldelim, const char spacer, const char rdelim);
const Token* parse_expression(const char rdelim);
const Token* parse_function_call();
const Token* parse_identifier();
const Token* parse_initializer_list();
const Token* parse_parameter_list();
const Token* parse_parenthesized_expression();
const Token* parse_specifiers();
const Token* parse_statement();

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

const Token* skip_lexemes(int count) {
  token = token + count;
  return token;
}

const Token* skip_punct(const char punct) {
  assert (token->is_punct());
  token++;
  return token;
}

const Token* skip_opt_punct(const char punct) {
  if (token->is_punct(punct)) token++;
  return token;
}

const Token* skip_identifier(const char* identifier = nullptr) {
  assert(token->is_identifier(identifier));
  token++;
  return token;
}

//----------------------------------------

const Token* take_lexemes(NodeType type, int count) {
  push_node( new Node(type, token, token + count) );
  token = token + count;
  return token;
}

const Token* take_punct(char punct) {
  if (token->is_punct(punct)) {
    return take_lexemes(NODE_PUNCT, 1);
  }
  else {
    return nullptr;
  }
}

const Token* take_identifier(const char* identifier = nullptr) {
  if (token->is_identifier(identifier)) {
    push_node( new Node(NODE_IDENTIFIER, token, token + 1) );
    token = token + 1;
    return token;
  }
  else {
    return nullptr;
  }
}

const Token* take_constant() {
  assert(token->is_constant());
  //return take_lexemes(NODE_CONSTANT, 1);
  if (take_lexemes(NODE_CONSTANT, 1)) {
    return token;
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
    push_node(new Node(NODE_ACCESS_SPECIFIER, a, end));
    return end;
  }
  else {
    return nullptr;
  }
}

//----------------------------------------

const Token* parse_declaration(const char rdelim) {
  auto result = new Node(NODE_INVALID);

  bool is_constructor = false;
  bool has_type = false;
  bool has_params = false;
  bool has_body = false;
  bool has_init = false;

  if (parse_specifiers()) {
    result->append(pop_node());
  }

  if (parse_enum_declaration()) {
    return token;
  }


  if (token[0].is_identifier() && token[1].is_punct('(')) {
    is_constructor = true;
    has_type = false;
    parse_identifier();
    result->append(pop_node());
  }
  else {
    // Need a better way to handle this
    parse_decltype();
    auto n1 = pop_node();

    if (token[0].is_punct('=')) {
      has_type = false;
      n1->node_type = NODE_IDENTIFIER;
      result->append(n1);
    }
    else {
      has_type = true;
      result->append(n1);
      if (parse_specifiers()) {
        result->append(pop_node());
      }
      if (parse_identifier()) {
        result->append(pop_node());
      }
    }

    if (take_punct(':')) {
      result->append(pop_node());
      if (parse_expression('{')) {
        result->append(pop_node());
      }
    }
  }

  while (token[0].is_punct('[')) {
    if (take_punct('[')) {
      result->append(pop_node());
    }

    if (parse_expression(']')) {
      result->append(pop_node());
    }

    if (take_punct(']')) {
      result->append(pop_node());
    }
  }

  if (token[0].is_punct('(')) {
    has_params = true;
    if (parse_parameter_list()) {
      result->append(pop_node());
    }

    if (is_constructor) {
      if (parse_initializer_list()) {
        result->append(pop_node());
      }
    }

    // grab that const after the param list
    if (parse_specifiers()) {
      result->append(pop_node());
    }
  }

  if (token[0].is_punct('{')) {
    has_body = true;
    parse_compound_statement();
    result->append(pop_node());
  }

  if (take_punct('=')) {
    has_init = true;
    result->append(pop_node());
    parse_expression(rdelim);
    result->append(pop_node());
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

  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_field_declaration_list() {
  auto result = new NodeList(NODE_FIELD_DECLARATION_LIST);
  skip_punct('{');

  while(token < token_eof && !token->is_punct('}')) {

    if (auto end = parse_access_specifier(token, token_eof)) {
      result->append(pop_node());
      token = end;
    }

    else if (parse_declaration(';')) {
      auto child = pop_node();
      result->append(child);
      result->items.push_back(child);
      if (take_punct(';')) {
        result->append(pop_node());
      }
    }
  }

  push_node(result);
  skip_punct('}');
  return token;
}

//----------------------------------------

const Token* parse_class_specifier() {

  using decl = Seq<AtomLit<"class">, Atom<LEX_IDENTIFIER>, Atom<';'>>;

  if (auto end = decl::match(token, token_eof)) {
    skip_identifier("class");
    take_identifier();
    auto name = pop_node();
    skip_punct(';');

    push_node(new ClassDeclaration(name));
    return token;
  }
  else {
    skip_identifier("class");
    take_identifier();
    auto name = pop_node();
    parse_field_declaration_list();
    auto body = pop_node();
    skip_punct(';');

    push_node(new ClassDefinition(name, body));
    return token;
  }

}

//----------------------------------------

const Token* parse_struct_specifier() {

  using decl = Seq<AtomLit<"struct">, Atom<LEX_IDENTIFIER>, Atom<';'>>;

  if (auto end = decl::match(token, token_eof)) {
    skip_identifier("struct");
    take_identifier();
    auto name = pop_node();
    skip_punct(';');

    push_node(new StructDeclaration(name));
    return token;
  }
  else {
    skip_identifier("struct");
    take_identifier();
    auto name = pop_node();
    parse_field_declaration_list();
    auto body = pop_node();
    skip_punct(';');
    push_node(new StructDefinition(name, body));
    return token;
  }

}

const Token* parse_namespace_specifier() {

  using decl = Seq<AtomLit<"namespace">, Atom<LEX_IDENTIFIER>, Atom<';'>>;

  if (auto end = decl::match(token, token_eof)) {
    skip_identifier("namespace");
    take_identifier();
    auto name = pop_node();
    skip_punct(';');
    auto result = new Node(NODE_NAMESPACE_DECLARATION);
    result->append(name);
    push_node(result);
    return token;
  }
  else {
    skip_identifier("namespace");
    take_identifier();
    auto name = pop_node();
    parse_field_declaration_list();
    auto body = pop_node();
    skip_punct(';');
    auto result = new Node(NODE_NAMESPACE_DEFINITION);
    result->append(name);
    result->append(body);
    push_node(result);
    return token;
  }
}

//----------------------------------------

const Token* parse_compound_statement() {
  if (!token->is_punct('{')) return nullptr;

  auto result = new CompoundStatement();

  skip_punct('{');

  while(1) {
    if (token->is_punct('}')) break;
    parse_statement();
    auto statement = pop_node();
    assert(statement);
    result->statements.push_back(statement);
    result->append(statement);
  }

  skip_punct('}');

  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_specifiers() {
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
        take_identifier();
        specifiers.push_back(pop_node());
        break;
      }
    }
    if (take_punct('*')) {
      specifiers.push_back(pop_node());
      match = true;
    }
    if (!match) break;
  }

  if (specifiers.size()) {
    auto result = new Node(NODE_SPECIFIER_LIST);
    for (auto n : specifiers) result->append(n);
    push_node(result);
    return token;
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

const Token* parse_decltype() {
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
    if (parse_expression_list(NODE_ARGUMENT_LIST, '<', ',', '>')) {
      result->append(pop_node());
    }

    if (token[0].is_punct(':') && token[1].is_punct(':')) {
      auto result2 = new Node(NODE_SCOPED_TYPE);
      result2->append(result);

      //result2->append(take_lexemes(NODE_OPERATOR, 2));
      if (take_lexemes(NODE_OPERATOR, 2)) {
        result2->append(pop_node());
      }
      else {
      }

      if (parse_decltype()) {
        result2->append(pop_node());
      }
      push_node(result2);
      return token;
    }

    push_node(result);
    return token;
  }
  else {
    push_node(type);
    return token;
  }
}

//----------------------------------------

const Token* parse_expression_prefix() {
  auto span_a = token->lex->span_a;
  auto span_b = token_eof->lex->span_a;

  if (auto end = match_prefix_op(span_a, span_b)) {
    auto match_lex_a = token;
    auto match_lex_b = token;
    while(match_lex_b->lex->span_a < end) match_lex_b++;
    auto result = new Node(NODE_OPERATOR, match_lex_a, match_lex_b);
    token = match_lex_b;
    push_node(result);
    return token;
  }

  if (token[0].is_punct('(') &&
      token[1].is_identifier() &&
      token[2].is_punct(')')) {
    auto result = new Node(NODE_TYPECAST);
    skip_punct('(');
    take_identifier();
    result->append(pop_node());
    skip_punct(')');
    push_node(result);
    return token;
  }

  if (token[0].is_identifier("sizeof")) {
    auto result = new Node(NODE_OPERATOR);
    take_identifier("sizeof");
    result->append(pop_node());
    push_node(result);
    return token;
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


const Token* parse_expression_suffix() {
  if (token[0].is_punct('[')) {
    auto result = new Node(NODE_ARRAY_SUFFIX);
    skip_punct('[');
    parse_expression(']');
    result->append(pop_node());
    skip_punct(']');
    push_node(result);
    return token;
  }

  if (token[0].is_punct('(') && token[1].is_punct(')')) {
    auto result = new Node(NODE_PARAMETER_LIST);
    skip_punct('(');
    skip_punct(')');
    push_node(result);
    return token;
  }

  if (token[0].is_punct('(')) {
    auto result = new Node(NODE_PARAMETER_LIST);
    skip_punct('(');
    parse_expression(')');
    result->append(pop_node());
    skip_punct(')');
    push_node(result);
    return token;
  }

  if (token[0].is_punct('+') && token[1].is_punct('+')) {
    //return take_lexemes(NODE_OPERATOR, 2);
    if (take_lexemes(NODE_OPERATOR, 2)) {
      return token;
    }
    else {
      return nullptr;
    }
  }

  if (token[0].is_punct('-') && token[1].is_punct('-')) {
    //return take_lexemes(NODE_OPERATOR, 2);
    if (take_lexemes(NODE_OPERATOR, 2)) {
      return token;
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

const Token* parse_assignment_op() {
  auto span_a = token->lex->span_a;
  auto span_b = token_eof->lex->span_a;

  auto end = match_assign_op(span_a, span_b);
  if (!end) return nullptr;

  auto match_lex_a = token;
  auto match_lex_b = token;
  while(match_lex_b->lex->span_a < end) match_lex_b++;
  auto result = new Node(NODE_OPERATOR, match_lex_a, match_lex_b);
  token = match_lex_b;
  push_node(result);
  return token;
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
  push_node(result);
  return token;
}

//----------------------------------------

Node* parse_expression_rhs(Node* lhs, const char rdelim) {
  if (token->is_eof())         return lhs;
  if (token->is_punct(')'))    return lhs;
  if (token->is_punct(';'))    return lhs;
  if (token->is_punct(','))    return lhs;
  if (token->is_punct(rdelim)) return lhs;

  if (parse_expression_suffix()) {
    auto op = pop_node();
    auto result = new Node(NODE_POSTFIX_EXPRESSION);
    result->append(lhs);
    result->append(op);
    return parse_expression_rhs(result, rdelim);
  }

  if (auto end = parse_infix_op()) {
    auto op = pop_node();
    parse_expression(rdelim);
    auto rhs = pop_node();

    auto result = new Node(NODE_INFIX_EXPRESSION);
    result->append(lhs);
    result->append(op);
    result->append(rhs);
    return parse_expression_rhs(result, rdelim);
  }

  if (token[0].is_punct('[')) {
    auto result = new Node(NODE_ARRAY_EXPRESSION);
    result->append(lhs);
    skip_punct('[');
    parse_expression(']');
    result->append(pop_node());
    skip_punct(']');
    return parse_expression_rhs(result, rdelim);
  }

  // Nothing found to continue the expression with?
  assert(false);
  return lhs;
}

//----------------------------------------

const Token* parse_expression_lhs(const char rdelim) {

  // Dirty hackkkkk - explicitly recognize templated function calls as
  // expression atoms
  if (token[0].is_identifier() &&
      token[1].is_punct('<') &&
      (token[2].is_identifier() || token[2].is_constant()) &&
      token[3].is_punct('>')) {
    auto result = new Node(NODE_CALL_EXPRESSION, nullptr, nullptr);

    if (take_identifier()) {
      result->append(pop_node());
    }

    if (parse_expression_list(NODE_TEMPLATE_PARAMETER_LIST, '<', ',', '>')) {
      result->append(pop_node());
    }

    if (parse_expression_list(NODE_ARGUMENT_LIST, '(', ',', ')')) {
      result->append(pop_node());
    }
    push_node(result);
    return token;
  }

  if (parse_expression_prefix()) {
    auto op = pop_node();
    auto result = new Node(NODE_PREFIX_EXPRESSION);
    result->append(op);
    parse_expression_lhs(rdelim);
    result->append(pop_node());
    push_node(result);
    return token;
  }

  if (token->is_punct('(')) {
    parse_parenthesized_expression();
    return token;
  }

  if (token->is_constant()) {
    take_constant();
    return token;
  }

  if (token[0].is_identifier() && token[1].is_punct('(')) {
    parse_function_call();
    return token;
  }

  if (token[0].is_identifier()) {
    take_identifier();
    return token;
  }

  assert(false);
  return nullptr;
}

//----------------------------------------

const Token* parse_expression(const char rdelim) {

  // FIXME there are probably other expression terminators?
  if (token->is_eof())      return nullptr;
  if (token->is_punct(')')) return nullptr;
  if (token->is_punct(';')) return nullptr;
  if (token->is_punct(',')) return nullptr;
  if (token->is_punct(rdelim)) return nullptr;

  parse_expression_lhs(rdelim);
  Node* lhs = pop_node();
  if (!lhs) return nullptr;

  if (token->is_eof())         { push_node(lhs); return token; }
  if (token->is_punct(')'))    { push_node(lhs); return token; }
  if (token->is_punct(';'))    { push_node(lhs); return token; }
  if (token->is_punct(','))    { push_node(lhs); return token; }
  if (token->is_punct(rdelim)) { push_node(lhs); return token; }

  auto result = parse_expression_rhs(lhs, rdelim);




  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_external_declaration() {
  if (token->is_eof()) {
    return nullptr;
  }

  if (token->is_identifier("namespace")) {
    parse_namespace_specifier();
    return token;
  }

  if (token->is_identifier("class")) {
    parse_class_specifier();
    return token;
  }

  if (token->is_identifier("struct")) {
    parse_struct_specifier();
    return token;
  }

  if (parse_declaration(';')) {
    skip_punct(';');
    return token;
  }

  return nullptr;
}

//----------------------------------------

const Token* parse_enum_declaration() {
  if (!token[0].is_identifier("enum")) return nullptr;

  Node* result = new Node(NODE_ENUM_DECLARATION);

  skip_identifier("enum");

  if (token[0].is_identifier("class")) {
    skip_identifier("class");
  }

  if (token[0].is_identifier()) {
    take_identifier();
    result->append(pop_node());
  }

  if (token[0].is_punct(':')) {
    skip_punct(':');
    if (parse_decltype()) {
      result->append(pop_node());
    }
  }

  if (token[0].is_punct('{')) {
    if (parse_expression_list(NODE_ENUMERATOR_LIST, '{', ',', '}')) {
      result->append(pop_node());
    }
  }

  // this is the weird enum {} blah;
  if (token[0].is_identifier()) {
    auto result2 = new Node(NODE_DECLARATION);
    result2->append(result);
    take_identifier();
    result2->append(pop_node());
    push_node(result2);
    return token;
  }

  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_function_call() {
  if (!token[0].is_identifier() || !token[1].is_punct('(')) return nullptr;

  auto result = new Node(NODE_CALL_EXPRESSION, nullptr, nullptr);

  if (take_identifier()) {
    result->append(pop_node());
  }
  if (parse_expression_list(NODE_ARGUMENT_LIST, '(', ',', ')')) {
    result->append(pop_node());
  }

  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_identifier() {
  if (token->type != TOK_IDENTIFIER) return nullptr;
  auto result = new Node(NODE_IDENTIFIER, token, token + 1);
  push_node(result);
  token = token + 1;
  return token;
}

//----------------------------------------

const Token* parse_if_statement() {
  auto result = new Node(NODE_IF_STATEMENT);
  if (take_identifier("if")) {
    result->append(pop_node());
  }
  if (parse_parenthesized_expression()) {
    result->append(pop_node());
  }
  if (parse_statement()) {
    result->append(pop_node());
  }
  if (token[0].is_identifier("else")) {
    take_identifier("else");
    result->append(pop_node());
    if (parse_statement()) {
      result->append(pop_node());
    }
  }
  push_node(result);
  return token;
}

const Token* parse_while_statement() {
  auto result = new Node(NODE_WHILE_STATEMENT);
  if (take_identifier("while")) {
    result->append(pop_node());
  }
  if (parse_parenthesized_expression()) {
    result->append(pop_node());
  }
  if (parse_statement()) {
    result->append(pop_node());
  }
  push_node(result);
  return token;
}

const Token* parse_return_statement() {
  take_identifier();
  auto ret = pop_node();
  parse_expression(';');
  auto val = pop_node();
  skip_punct(';');
  auto result = new Node(NODE_RETURN_STATEMENT, nullptr, nullptr);
  result->append(ret);
  result->append(val);
  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_parameter_list() {
  auto result = new NodeList(NODE_PARAMETER_LIST);

  skip_punct('(');

  while(!token->is_punct(')')) {
    parse_declaration(',');
    auto decl = pop_node();
    assert(decl);
    result->items.push_back(decl);
    result->append(decl);
    if (token->is_punct(',')) {
      skip_punct(',');
    }
  }

  skip_punct(')');

  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_parenthesized_expression() {
  auto result = new Node(NODE_PARENTHESIZED_EXPRESSION);
  skip_punct('(');
  parse_expression(')');
  result->append(pop_node());
  skip_punct(')');
  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_preproc() {
  using pattern = Atom<TOK_PREPROC>;

  if (auto end = pattern::match(token, token_eof)) {
    auto result = new Node(NODE_PREPROC, token, end);
    token = end;
    push_node(result);
    return token;
  }

  return nullptr;
}

//----------------------------------------

const Token* parse_case_statement() {
  if (!token[0].is_case_label()) return nullptr;

  Node* result = new Node(NODE_CASE_STATEMENT);

  if (token[0].is_identifier("case")) {
    take_identifier();
    result->append(pop_node());
    parse_expression(':');
    result->append(pop_node());
    skip_punct(':');
  }
  else if (token[0].is_identifier("default")) {
    take_identifier();
    result->append(pop_node());
    skip_punct(':');
  }
  else {
    assert(false);
    return nullptr;
  }

  while (!token[0].is_case_label() && !token[0].is_punct('}')) {
    if (parse_statement()) {
      result->append(pop_node());
    }
  }

  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_switch_statement() {
  if (!token[0].is_identifier("switch")) return nullptr;

  Node* result = new Node(NODE_SWITCH_STATEMENT);

  if (take_identifier("switch")) {
    result->append(pop_node());
  }

  if (parse_expression_list(NODE_ARGUMENT_LIST, '(', ',', ')')) {
    result->append(pop_node());
  }
  skip_punct('{');

  while(!token[0].is_punct('}')) {
    parse_case_statement();
    result->append(pop_node());
  }

  skip_punct('}');

  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_declaration_or_expression(char rdelim) {
  if (parse_declaration(rdelim)) {
    return token;
  }
  if (parse_expression(rdelim)) {
    return token;
  }
  return nullptr;
}

//----------------------------------------

const Token* parse_for_statement() {
  if (!token[0].is_identifier("for")) return nullptr;

  auto result = new Node(NODE_FOR_STATEMENT);

  result->tok_a = token;

  auto old_size = node_top;

  skip_identifier("for");
  skip_punct('(');
  parse_declaration_or_expression(';');
  result->append(pop_node());
  skip_punct(';');
  parse_expression(';');
  result->append(pop_node());
  skip_punct(';');
  parse_expression(')');
  result->append(pop_node());
  skip_punct(')');
  if (parse_statement()) {
    result->append(pop_node());
  }

  auto new_size = node_top;

  result->tok_b = token;

  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_statement() {
  if (token[0].is_punct('{')) {
    parse_compound_statement();
    return token;
  }

  if (token[0].is_identifier("if")) {
    parse_if_statement();
    return token;
  }

  if (token[0].is_identifier("while")) {
    parse_while_statement();
    return token;
  }

  if (token[0].is_identifier("for")) {
    parse_for_statement();
    return token;
  }

  if (token[0].is_identifier("return")) {
    parse_return_statement();
    return token;
  }

  if (token[0].is_identifier("switch")) {
    parse_switch_statement();
    return token;
  }

  if (token[0].is_identifier() && token[1].is_identifier()) {
    auto result = new Node(NODE_DECLARATION_STATEMENT, nullptr, nullptr);
    if (parse_declaration(';')) {
      result->append(pop_node());
    }
    skip_punct(';');
    push_node(result);
    return token;
  }

  // Dirty hack
  if (token[0].is_identifier() &&
      token[1].is_punct('<') &&
      (token[2].is_identifier() || token[2].is_constant()) &&
      token[3].is_punct('>') &&
      token[4].is_identifier()) {
    auto result = new Node(NODE_DECLARATION_STATEMENT, nullptr, nullptr);
    if (parse_declaration(';')) {
      result->append(pop_node());
    }
    skip_punct(';');
    push_node(result);
    return token;
  }

  // Must be expression statement
  parse_expression(';');
  auto exp = pop_node();
  skip_punct(';');
  auto result = new Node(NODE_EXPRESSION_STATEMENT, nullptr, nullptr);
  result->append(exp);
  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_storage_class_specifier() {
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
      take_identifier();
      return token;
    }
  }

  return nullptr;
}

//----------------------------------------

const Token* parse_template_decl() {
  auto result = new TemplateDeclaration();

  if (take_identifier("template")) {
    result->append(pop_node());
  }
  if (parse_declaration_list(NODE_TEMPLATE_PARAMETER_LIST, '<', ',', '>')) {
    result->append(pop_node());
  }
  if (parse_class_specifier()) {
    result->append(pop_node());
  }

  result->_keyword = result->child(0);
  result->_params  = result->child(1);
  result->_class   = result->child(2);

  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_declaration_list(NodeType type, const char ldelim, const char spacer, const char rdelim) {
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
      parse_declaration(rdelim);
      result->append(pop_node());
    }
  }

  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_expression_list(NodeType type, const char ldelim, const char spacer, const char rdelim) {
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
      parse_expression(rdelim);
      result->append(pop_node());
    }
  }

  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_initializer_list() {
  char ldelim = ':';
  char spacer = ',';
  char rdelim = '{'; // we don't consume this unlike parse_expression_list

  if (!token[0].is_punct(ldelim)) return nullptr;

  auto result = new Node(NODE_INITIALIZER_LIST);

  skip_punct(ldelim);

  while(1) {
    if (token->is_punct(rdelim)) {
      push_node(result);
      return token;
    }

    if (token->is_punct(spacer)) {
      skip_punct(spacer);
    }
    else {
      parse_expression(rdelim);
      result->append(pop_node());
    }
  }

  push_node(result);
  return token;
}

//----------------------------------------

const Token* parse_type_qualifier() {
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
      take_identifier();
      return token;
    }
  }

  assert(false);
  return nullptr;
}

//----------------------------------------

const Token* parse_translation_unit() {
  auto result = new TranslationUnit();

  while(!token->is_eof()) {

    if (Lit<"template">::match(token->lex->span_a, token_eof->lex->span_a)) {
      if (parse_template_decl()) {
        result->append(pop_node());
      }
    }
    else if (parse_preproc()) {
      result->append(pop_node());
    }
    else if (parse_external_declaration()) {
      auto decl = pop_node();
      result->append(decl);
    }
    else {
      assert(false);
      break;
    }
  }

  push_node(result);
  return token;
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

    parse_translation_unit();
    auto root = pop_node();
    //printf("\n");
    //root->dump_tree();
    //printf("\n");
    delete root;

  }

  auto time_b = timestamp_ms();

  printf("Parsing took %f msec\n", time_b - time_a);

  return 0;
}

//------------------------------------------------------------------------------
