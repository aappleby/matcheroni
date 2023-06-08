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

bool atom_eq(const Token& a, const LexemeType& b) {
  return a.lex->type == b;
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

//----------------------------------------

template<typename CharMatcher>
const Token* match_chars(const Token* a, const Token* b) {
  auto span_a = a->lex->span_a;
  auto span_b = b->lex->span_a;

  auto end = CharMatcher::match(span_a, span_b);
  if (!end) return nullptr;

  auto match_lex_a = a;
  auto match_lex_b = a;
  while(match_lex_b->lex->span_a < end) match_lex_b++;
  return match_lex_b;
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

Node* node_stack[256] = {0};
size_t node_top = 0;

Node* pop_node()         { return node_stack[--node_top]; }
void  push_node(Node* n) { node_stack[node_top++] = n; }

const Token* parse_compound_statement(const Token* a, const Token* b);
const Token* parse_declaration_list(const Token* a, const Token* b, NodeType type, const char ldelim, const char spacer, const char rdelim);
const Token* parse_decltype(const Token* a, const Token* b);
const Token* parse_enum_declaration(const Token* a, const Token* b);
const Token* parse_expression_list(const Token* a, const Token* b, NodeType type, const char ldelim, const char spacer, const char rdelim);
const Token* parse_expression2(const Token* a, const Token* b);
const Token* parse_expression(const Token* a, const Token* b, const char rdelim = 0);
const Token* parse_function_call(const Token* a, const Token* b);
const Token* parse_identifier(const Token* a, const Token* b);
const Token* parse_initializer_list(const Token* a, const Token* b);
const Token* parse_parameter_list(const Token* a, const Token* b);
const Token* parse_parenthesized_expression(const Token* a, const Token* b);
const Token* parse_specifiers(const Token* a, const Token* b);
const Token* parse_statement(const Token* a, const Token* b);

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

//------------------------------------------------------------------------------

const Token* skip_punct(const Token* a, const Token* b, const char punct) {
  return a->is_punct(punct) ? a + 1 : nullptr;
}

const Token* skip_identifier(const Token* a, const Token* b, const char* identifier = nullptr) {
  return a->is_identifier(identifier) ? a + 1 : nullptr;
}

//----------------------------------------

template<NodeType n, typename pattern>
struct NodeMaker {
  static const Token* match(const Token* a, const Token* b) {
    auto old_top = node_top;
    auto end = pattern::match(a, b);
    if (end) {
      auto node = new Node(n, a, end);
      for (auto i = old_top; i < node_top; i++) {
        node->append(node_stack[i]);
      }
      node_top = old_top;
      push_node(node);
    }
    else {
      for (auto i = old_top; i < node_top; i++) {
        delete node_stack[i];
      }
      node_top = old_top;
    }
    return end;
  }
};

//----------------------------------------

const Token* take_lexemes(const Token* a, const Token* b, NodeType type, int count) {
  push_node( new Node(type, a, a + count) );
  return a + count;
}

template<char punct>
const Token* take_punct2(const Token* a, const Token* b) {
  using pattern = NodeMaker<NODE_PUNCT, Atom<punct>>;
  return pattern::match(a, b);
}

const Token* take_punct(const Token* a, const Token* b, char punct) {
  if (!a->is_punct(punct)) return nullptr;
  return take_lexemes(a, b, NODE_PUNCT, 1);
}

const Token* take_identifier(const Token* a, const Token* b, const char* identifier = nullptr) {
  if (!a->is_identifier(identifier)) return nullptr;
  push_node( new Node(NODE_IDENTIFIER, a, a + 1) );
  return a + 1;
}

const Token* take_any_identifier(const Token* a, const Token* b) {
  using pattern = NodeMaker<NODE_IDENTIFIER, Atom<LEX_IDENTIFIER>>;
  return pattern::match(a, b);
}

const Token* take_constant(const Token* a, const Token* b) {
  using pattern = NodeMaker<NODE_CONSTANT, Oneof<
    Atom<TOK_FLOAT>,
    Atom<TOK_INT>,
    Atom<TOK_CHAR>,
    Atom<TOK_STRING>
  >>;

  return pattern::match(a, b);
}

//----------------------------------------

using pattern_access_specifier = NodeMaker<
  NODE_ACCESS_SPECIFIER,
  Seq<
    Oneof<AtomLit<"public">, AtomLit<"private">>,
    Atom<':'>
  >
>;

//----------------------------------------

using pattern_initializer_list = NodeMaker<NODE_INITIALIZER_LIST, Seq<
  Atom<':'>,
  Ref<parse_expression2>,
  Any<Seq<Atom<','>, Ref<parse_expression2>>>,
  And<Atom<'{'>>
>>;

//----------------------------------------

const Token* parse_declaration(const Token* a, const Token* b, const char rdelim) {
  auto result = new Node(NODE_INVALID);

  bool is_constructor = false;
  bool has_type = false;
  bool has_params = false;
  bool has_body = false;
  bool has_init = false;

  if (auto end = parse_specifiers(a, b)) {
    a = end;
    result->append(pop_node());
  }

  if (auto end = parse_enum_declaration(a, b)) {
    a = end;
    return a;
  }


  if (a[0].is_identifier() && a[1].is_punct('(')) {
    is_constructor = true;
    has_type = false;
    if (auto end = parse_identifier(a, b)) {
      a = end;
    }
    result->append(pop_node());
  }
  else {
    // Need a better way to handle this
    if (auto end = parse_decltype(a, b)) {
      a = end;
    }
    auto n1 = pop_node();

    if (a[0].is_punct('=')) {
      has_type = false;
      n1->node_type = NODE_IDENTIFIER;
      result->append(n1);
    }
    else {
      has_type = true;
      result->append(n1);
      if (auto end = parse_specifiers(a, b)) {
        a = end;
        result->append(pop_node());
      }
      if (auto end = parse_identifier(a, b)) {
        a = end;
        result->append(pop_node());
      }
    }

    if (auto end = skip_punct(a, b, ':')) {
      a = end;

      if (auto end = parse_expression(a, b, '{')) {
        a = end;
        result->append(pop_node());
      }
    }
  }

  while (a[0].is_punct('[')) {
    if (auto end = skip_punct(a, b, '[')) {
      a = end;
    }

    if (auto end = parse_expression(a, b, ']')) {
      a = end;
      result->append(pop_node());
    }

    if (auto end = skip_punct(a, b, ']')) {
      a = end;
    }
  }

  if (a[0].is_punct('(')) {
    has_params = true;
    if (auto end = parse_parameter_list(a, b)) {
      a = end;
      result->append(pop_node());
    }

    if (is_constructor) {
      if (auto end = pattern_initializer_list::match(a, b)) {
        a = end;
        result->append(pop_node());
      }
    }

    // grab that const after the param list
    if (auto end = parse_specifiers(a, b)) {
      a = end;
      result->append(pop_node());
    }
  }

  if (a[0].is_punct('{')) {
    has_body = true;
    a = parse_compound_statement(a, b);
    result->append(pop_node());
  }

  if (auto end = skip_punct(a, b, '=')) {
    a = end;
    has_init = true;
    if (auto end = parse_expression(a, b, rdelim)) {
      a = end;
    }
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
  return a;
}

const Token* parse_declaration2(const Token* a, const Token* b) {
  return parse_declaration(a, b, ';');
}

//----------------------------------------

const Token* parse_field_declaration_list(const Token* a, const Token* b) {
  auto result = new NodeList(NODE_FIELD_DECLARATION_LIST);
  a = skip_punct(a, b, '{');

  while(a < b && !a->is_punct('}')) {

    if (auto end = pattern_access_specifier::match(a, b)) {
      result->append(pop_node());
      a = end;
    }
    else {
      if (auto end = parse_declaration(a, b, ';')) {
        a = end;
        auto child = pop_node();
        result->append(child);
        result->items.push_back(child);
        if (auto end = skip_punct(a, b, ';')) {
          a = end;
        }
      }
    }
  }

  push_node(result);
  a = skip_punct(a, b, '}');
  return a;
}

//----------------------------------------

const Token* parse_class_specifier(const Token* a, const Token* b) {
  if (!a->is_identifier("class")) return nullptr;

  using decl = Seq<AtomLit<"class">, Atom<LEX_IDENTIFIER>, Atom<';'>>;

  if (auto end = decl::match(a, b)) {
    a = skip_identifier(a, b, "class");
    a = take_identifier(a, b);
    auto name = pop_node();
    a = skip_punct(a, b, ';');

    push_node(new ClassDeclaration(name));
    return a;
  }
  else {
    a = skip_identifier(a, b, "class");
    a = take_identifier(a, b);
    auto name = pop_node();

    a = parse_field_declaration_list(a, b);

    auto body = pop_node();
    a = skip_punct(a, b, ';');

    push_node(new ClassDefinition(name, body));
    return a;
  }

}

//----------------------------------------

const Token* parse_struct_specifier(const Token* a, const Token* b) {
  if (!a->is_identifier("struct")) return nullptr;

  using decl = Seq<AtomLit<"struct">, Atom<LEX_IDENTIFIER>, Atom<';'>>;

  if (auto end = decl::match(a, b)) {
    a = skip_identifier(a, b, "struct");
    a = take_identifier(a, b);
    auto name = pop_node();
    a = skip_punct(a, b, ';');

    push_node(new StructDeclaration(name));
    return a;
  }
  else {
    a = skip_identifier(a, b, "struct");
    a = take_identifier(a, b);
    auto name = pop_node();
    a = parse_field_declaration_list(a, b);
    auto body = pop_node();
    a = skip_punct(a, b, ';');
    push_node(new StructDefinition(name, body));
    return a;
  }

}

const Token* parse_namespace_specifier(const Token* a, const Token* b) {
  if (!a->is_identifier("namespace")) return nullptr;

  using decl = Seq<AtomLit<"namespace">, Atom<LEX_IDENTIFIER>, Atom<';'>>;

  if (auto end = decl::match(a, b)) {
    a = skip_identifier(a, b, "namespace");
    a = take_identifier(a, b);
    a = skip_punct(a, b, ';');
    auto result = new Node(NODE_NAMESPACE_DECLARATION);
    result->append(pop_node());
    push_node(result);
    return a;
  }
  else {
    a = skip_identifier(a, b, "namespace");
    a = take_identifier(a, b);
    a = parse_field_declaration_list(a, b);
    a = skip_punct(a, b, ';');

    auto body = pop_node();
    auto name = pop_node();

    auto result = new Node(NODE_NAMESPACE_DEFINITION);
    result->append(name);
    result->append(body);
    push_node(result);
    return a;
  }
}

//----------------------------------------

const Token* parse_compound_statement(const Token* a, const Token* b) {
  if (!a->is_punct('{')) return nullptr;

  auto result = new CompoundStatement();
  push_node(result);

  auto old_top = node_top;

  a = skip_punct(a, b, '{');

  while(auto end = parse_statement(a, b)) {
    a = end;
  }

  a = skip_punct(a, b, '}');

  for (auto c = old_top; c < node_top; c++) {
    result->statements.push_back(node_stack[c]);
    result->append(node_stack[c]);
  }

  node_top = old_top;

  return a;
}

//----------------------------------------

const Token* match_specifier(const Token* a, const Token* b) {
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

  for (auto i = 0; i < keyword_count; i++) {
    if (a->is_identifier(keywords[i])) {
      return a + 1;
    }
  }
  return nullptr;
}

//----------------------------------------

const Token* parse_specifiers(const Token* a, const Token* b) {
  using pattern = NodeMaker<NODE_SPECIFIER_LIST,
    Some<Oneof<
      NodeMaker<NODE_IDENTIFIER, Ref<match_specifier>>,
      NodeMaker<NODE_PUNCT, Atom<'*'>>
    >>
  >;

  return pattern::match(a, b);
}

//----------------------------------------

const Token* parse_decltype(const Token* a, const Token* b) {
  if (a[0].type != TOK_IDENTIFIER) return nullptr;

  Node* type = nullptr;

  // FIXME do some proper type parsing here
  if (a[1].is_punct('*')) {
    type = new Node(NODE_DECLTYPE, &a[0], &a[2]);
    a += 2;
  }
  else {
    type = new Node(NODE_DECLTYPE, &a[0], &a[1]);
    a += 1;
  }

  if (a[0].is_punct('<')) {
    auto result = new Node(NODE_TEMPLATED_TYPE, nullptr, nullptr);
    result->append(type);

    if (auto end = parse_expression_list(a, b, NODE_ARGUMENT_LIST, '<', ',', '>')) {
      a = end;
      result->append(pop_node());
    }

    if (a[0].is_punct(':') && a[1].is_punct(':')) {
      auto result2 = new Node(NODE_SCOPED_TYPE);
      result2->append(result);

      a = take_lexemes(a, b, NODE_OPERATOR, 2);
      result2->append(pop_node());

      if (auto end = parse_decltype(a, b)) {
        a = end;
        result2->append(pop_node());
      }
      push_node(result2);
      return a;
    }

    push_node(result);
    return a;
  }
  else {
    push_node(type);
    return a;
  }
}

//----------------------------------------

const Token* parse_expression_prefix(const Token* a, const Token* b) {
  using pattern_prefix_op = NodeMaker<NODE_OPERATOR,
    Ref<match_chars<Ref<match_prefix_op>>>
  >;

  using pattern_typecast = NodeMaker<
    NODE_TYPECAST,
    Seq<Atom<'('>, Atom<LEX_IDENTIFIER>, Atom<')'> >
  >;

  using pattern_sizeof = NodeMaker<NODE_OPERATOR, AtomLit<"sizeof">>;

  using pattern = Oneof<
    pattern_prefix_op,
    pattern_typecast,
    pattern_sizeof
  >;

  return pattern::match(a, b);
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

const Token* parse_expression_suffix(const Token* a, const Token* b) {
  using pattern_array_suffix = NodeMaker<NODE_ARRAY_SUFFIX,
    Seq<Atom<'['>, Opt<Ref<parse_expression2>>, Atom<']'>>
  >;

  using pattern_parameter_list = NodeMaker<NODE_PARAMETER_LIST,
    Seq<Atom<'('>, Opt<Ref<parse_expression2>>, Atom<')'>>
  >;

  using pattern_inc = NodeMaker<NODE_OPERATOR, Seq<Atom<'+'>, Atom<'+'>>>;

  using pattern_dec = NodeMaker<NODE_OPERATOR, Seq<Atom<'-'>, Atom<'-'>>>;

  using pattern = Oneof<
    pattern_array_suffix,
    pattern_parameter_list,
    pattern_inc,
    pattern_dec
  >;

  return pattern::match(a, b);
}

//----------------------------------------

using pattern_infix_op = NodeMaker<NODE_OPERATOR,
  Ref<match_chars<Ref<match_infix_op>>>
>;

//----------------------------------------

const Token* parse_expression_lhs(const Token* a, const Token* b, const char rdelim) {

  // Dirty hackkkkk - explicitly recognize templated function calls as
  // expression atoms
  if (a[0].is_identifier() &&
      a[1].is_punct('<') &&
      (a[2].is_identifier() || a[2].is_constant()) &&
      a[3].is_punct('>')) {
    auto result = new Node(NODE_CALL_EXPRESSION, nullptr, nullptr);

    if (a = take_identifier(a, b)) {
      result->append(pop_node());
    }

    if (auto end = parse_expression_list(a, b, NODE_TEMPLATE_PARAMETER_LIST, '<', ',', '>')) {
      a = end;
      result->append(pop_node());
    }

    if (auto end = parse_expression_list(a, b, NODE_ARGUMENT_LIST, '(', ',', ')')) {
      a = end;
      result->append(pop_node());
    }
    push_node(result);
    return a;
  }

  if (auto end = parse_expression_prefix(a, b)) {
    a = end;
    auto op = pop_node();
    auto result = new Node(NODE_PREFIX_EXPRESSION);
    result->append(op);
    if (auto end = parse_expression_lhs(a, b, rdelim)) {
      a = end;
      result->append(pop_node());
    }
    push_node(result);
    return a;
  }

  using pattern = Oneof<
    Ref<parse_parenthesized_expression>,
    Ref<take_constant>,
    Ref<parse_function_call>,
    Ref<take_any_identifier>
  >;

  return pattern::match(a, b);
}

//----------------------------------------

const Token* parse_expression(const Token* a, const Token* b, const char rdelim) {

  // FIXME there are probably other expression terminators?
  if (a->is_eof())         return nullptr;
  if (a->is_punct(')'))    return nullptr;
  if (a->is_punct(';'))    return nullptr;
  if (a->is_punct(','))    return nullptr;
  if (a->is_punct(rdelim)) return nullptr;

  if (auto end = parse_expression_lhs(a, b, rdelim)) {
    a = end;
  }
  else {
    return nullptr;
  }

  if (a->is_eof())         { return a; }
  if (a->is_punct(')'))    { return a; }
  if (a->is_punct(';'))    { return a; }
  if (a->is_punct(','))    { return a; }
  if (a->is_punct(rdelim)) { return a; }

  if (auto end = parse_expression_suffix(a, b)) {
    a = end;
    auto op = pop_node();
    auto lhs = pop_node();
    auto result = new Node(NODE_POSTFIX_EXPRESSION);
    result->append(lhs);
    result->append(op);
    push_node(result);
    return a;
  }

  if (auto end = pattern_infix_op::match(a, b)) {
    a = end;
    auto op = pop_node();
    auto lhs = pop_node();
    a = parse_expression(a, b, rdelim);
    auto rhs = pop_node();

    auto result = new Node(NODE_INFIX_EXPRESSION);
    result->append(lhs);
    result->append(op);
    result->append(rhs);
    push_node(result);
    return a;
  }

  if (a[0].is_punct('[')) {
    auto result = new Node(NODE_ARRAY_EXPRESSION);
    auto lhs = pop_node();
    result->append(lhs);
    a = skip_punct(a, b, '[');

    a = parse_expression(a, b);

    result->append(pop_node());
    a = skip_punct(a, b, ']');
    push_node(result);
    return a;
  }

  return a;
}

const Token* parse_expression2(const Token* a, const Token* b) {
  return parse_expression(a, b, 0);
}

//----------------------------------------

using pattern_external_declaration = Oneof<
  Ref<parse_namespace_specifier>,
  Ref<parse_class_specifier>,
  Ref<parse_struct_specifier>,
  Seq<Ref<parse_declaration2>, Atom<';'>>
>;

//----------------------------------------

const Token* parse_enum_declaration(const Token* a, const Token* b) {
  if (!a[0].is_identifier("enum")) return nullptr;

  Node* result = new Node(NODE_ENUM_DECLARATION);

  a = skip_identifier(a, b, "enum");

  if (a[0].is_identifier("class")) {
    a = skip_identifier(a, b, "class");
  }

  if (a[0].is_identifier()) {
    a = take_identifier(a, b);
    result->append(pop_node());
  }

  if (a[0].is_punct(':')) {
    a = skip_punct(a, b, ':');

    if (auto end = parse_decltype(a, b)) {
      a = end;
      result->append(pop_node());
    }
  }

  if (a[0].is_punct('{')) {

    if (auto end = parse_expression_list(a, b, NODE_ENUMERATOR_LIST, '{', ',', '}')) {
      a = end;
      result->append(pop_node());
    }
  }

  // this is the weird enum {} blah;
  if (a[0].is_identifier()) {
    auto result2 = new Node(NODE_DECLARATION);
    result2->append(result);
    a = take_identifier(a, b);
    result2->append(pop_node());
    push_node(result2);
    return a;
  }

  push_node(result);
  return a;
}

//----------------------------------------

const Token* parse_function_call(const Token* a, const Token* b) {
  if (!a[0].is_identifier() || !a[1].is_punct('(')) return nullptr;

  auto result = new Node(NODE_CALL_EXPRESSION, nullptr, nullptr);

  if (a = take_identifier(a, b)) {
    result->append(pop_node());
  }
  if (auto end = parse_expression_list(a, b, NODE_ARGUMENT_LIST, '(', ',', ')')) {
    a = end;
    result->append(pop_node());
  }

  push_node(result);
  return a;
}

//----------------------------------------

const Token* parse_identifier(const Token* a, const Token* b) {
  if (a->type != TOK_IDENTIFIER) return nullptr;
  auto result = new Node(NODE_IDENTIFIER, a, a + 1);
  push_node(result);
  return a + 1;
}

//----------------------------------------

const Token* parse_if_statement(const Token* a, const Token* b) {
  auto result = new Node(NODE_IF_STATEMENT);
  if (auto end = take_identifier(a, b, "if")) {
    a = end;
    result->append(pop_node());
  }

  if (auto end = parse_parenthesized_expression(a, b)) {
    a = end;
    result->append(pop_node());
  }

  if (auto end = parse_statement(a, b)) {
    a = end;
    result->append(pop_node());
  }

  if (a[0].is_identifier("else")) {
    a = take_identifier(a, b, "else");
    result->append(pop_node());

    if (auto end = parse_statement(a, b)) {
      a = end;
      result->append(pop_node());
    }
  }

  push_node(result);
  return a;
}

const Token* parse_while_statement(const Token* a, const Token* b) {
  auto result = new Node(NODE_WHILE_STATEMENT);
  if (auto end = take_identifier(a, b, "while")) {
    a = end;
    result->append(pop_node());
  }

  if (auto end = parse_parenthesized_expression(a, b)) {
    a = end;
    result->append(pop_node());
  }

  if (auto end = parse_statement(a, b)) {
    a = end;
    result->append(pop_node());
  }

  push_node(result);
  return a;
}

const Token* parse_return_statement(const Token* a, const Token* b) {
  a = take_identifier(a, b);
  auto ret = pop_node();

  a = parse_expression(a, b, ';');

  auto val = pop_node();
  a = skip_punct(a, b, ';');
  auto result = new Node(NODE_RETURN_STATEMENT, nullptr, nullptr);
  result->append(ret);
  result->append(val);
  push_node(result);
  return a;
}

//----------------------------------------

const Token* parse_parameter_list(const Token* a, const Token* b) {
  auto result = new NodeList(NODE_PARAMETER_LIST);

  a = skip_punct(a, b, '(');

  while(!a->is_punct(')')) {

    if (auto end = parse_declaration(a, b, ',')) {
      a = end;
    }

    auto decl = pop_node();
    assert(decl);
    result->items.push_back(decl);
    result->append(decl);
    if (a->is_punct(',')) {
      a = skip_punct(a, b, ',');
    }
  }

  a = skip_punct(a, b, ')');

  push_node(result);
  return a;
}

//----------------------------------------

const Token* parse_parenthesized_expression(const Token* a, const Token* b) {
  if (!a->is_punct('(')) return nullptr;

  auto result = new Node(NODE_PARENTHESIZED_EXPRESSION);
  a = skip_punct(a, b, '(');

  a = parse_expression(a, b, ')');

  result->append(pop_node());
  a = skip_punct(a, b, ')');
  push_node(result);
  return a;
}

//----------------------------------------

using pattern_preproc = NodeMaker<NODE_PREPROC, Atom<TOK_PREPROC>>;


//----------------------------------------

const Token* parse_case_statement(const Token* a, const Token* b) {
  if (!a[0].is_case_label()) return nullptr;

  Node* result = new Node(NODE_CASE_STATEMENT);

  if (a[0].is_identifier("case")) {
    a = take_identifier(a, b);
    result->append(pop_node());
    a = parse_expression(a, b, ':');
    result->append(pop_node());
    a = skip_punct(a, b, ':');
  }
  else if (a[0].is_identifier("default")) {
    a = take_identifier(a, b);
    result->append(pop_node());
    a = skip_punct(a, b, ':');
  }
  else {
    assert(false);
    return nullptr;
  }

  while (!a[0].is_case_label() && !a[0].is_punct('}')) {
    if (auto end = parse_statement(a, b)) {
      a = end;
      result->append(pop_node());
    }
  }

  push_node(result);
  return a;
}

//----------------------------------------

const Token* parse_switch_statement(const Token* a, const Token* b) {
  if (!a[0].is_identifier("switch")) return nullptr;

  Node* result = new Node(NODE_SWITCH_STATEMENT);

  if (auto end = take_identifier(a, b, "switch")) {
    a = end;
    result->append(pop_node());
  }

  if (auto end = parse_expression_list(a, b, NODE_ARGUMENT_LIST, '(', ',', ')')) {
    a = end;
    result->append(pop_node());
  }


  a = skip_punct(a, b, '{');

  while(!a[0].is_punct('}')) {
    a = parse_case_statement(a, b);
    result->append(pop_node());
  }

  a = skip_punct(a, b, '}');

  push_node(result);
  return a;
}

//----------------------------------------

const Token* parse_declaration_or_expression(const Token* a, const Token* b, char rdelim) {
  if (auto end = parse_declaration(a, b, rdelim)) {
    a = end;
    return a;
  }
  if (auto end = parse_expression(a, b, rdelim)) {
    a = end;
    return a;
  }
  return nullptr;
}

//----------------------------------------

const Token* parse_for_statement(const Token* a, const Token* b) {
  if (!a[0].is_identifier("for")) return nullptr;

  auto result = new Node(NODE_FOR_STATEMENT);

  result->tok_a = a;

  auto old_size = node_top;

  a = skip_identifier(a, b, "for");
  a = skip_punct(a, b, '(');

  a = parse_declaration_or_expression(a, b, ';');

  result->append(pop_node());
  a = skip_punct(a, b, ';');

  a = parse_expression(a, b, ';');

  result->append(pop_node());
  a = skip_punct(a, b, ';');

  a = parse_expression(a, b, ')');

  result->append(pop_node());
  a = skip_punct(a, b, ')');

  if (auto end = parse_statement(a, b)) {
    a = end;
    result->append(pop_node());
  }

  auto new_size = node_top;

  result->tok_b = a;

  push_node(result);
  return a;
}

//----------------------------------------

const Token* parse_statement(const Token* a, const Token* b) {
  if (a[0].is_punct('}')) {
    return nullptr;
  }

  if (a[0].is_punct('{')) {
    a = parse_compound_statement(a, b);
    return a;
  }

  if (a[0].is_identifier("if")) {
    a = parse_if_statement(a, b);
    return a;
  }

  if (a[0].is_identifier("while")) {
    a = parse_while_statement(a, b);
    return a;
  }

  if (a[0].is_identifier("for")) {
    a = parse_for_statement(a, b);
    return a;
  }

  if (a[0].is_identifier("return")) {
    a = parse_return_statement(a, b);
    return a;
  }

  if (a[0].is_identifier("switch")) {
    a = parse_switch_statement(a, b);
    return a;
  }

  if (a[0].is_identifier() && a[1].is_identifier()) {
    auto result = new Node(NODE_DECLARATION_STATEMENT, nullptr, nullptr);

    if (auto end = parse_declaration(a, b, ';')) {
      a = end;
      result->append(pop_node());
    }

    a = skip_punct(a, b, ';');
    push_node(result);
    return a;
  }

  // Dirty hack
  if (a[0].is_identifier() &&
      a[1].is_punct('<') &&
      (a[2].is_identifier() || a[2].is_constant()) &&
      a[3].is_punct('>') &&
      a[4].is_identifier()) {
    auto result = new Node(NODE_DECLARATION_STATEMENT, nullptr, nullptr);

    if (auto end = parse_declaration(a, b, ';')) {
      a = end;
      result->append(pop_node());
    }

    a = skip_punct(a, b, ';');
    push_node(result);
    return a;
  }

  // Must be expression statement
  a = parse_expression(a, b, ';');

  auto exp = pop_node();
  a = skip_punct(a, b, ';');
  auto result = new Node(NODE_EXPRESSION_STATEMENT, nullptr, nullptr);
  result->append(exp);
  push_node(result);
  return a;
}

//----------------------------------------

const Token* parse_storage_class_specifier(const Token* a, const Token* b) {
  const char* keywords[] = {
    "extern",
    "static",
    "auto",
    "register",
    "inline",
  };
  auto keyword_count = sizeof(keywords)/sizeof(keywords[0]);
  for (auto i = 0; i < keyword_count; i++) {
    if (a->is_identifier(keywords[i])) {
      return take_identifier(a, b);
    }
  }

  return nullptr;
}

//----------------------------------------

const Token* parse_template_decl(const Token* a, const Token* b) {
  auto result = new TemplateDeclaration();

  if (auto end = take_identifier(a, b, "template")) {
    a = end;
    result->append(pop_node());
  }
  else {
    return nullptr;
  }

  if (auto end = parse_declaration_list(a, b, NODE_TEMPLATE_PARAMETER_LIST, '<', ',', '>')) {
    a = end;
    result->append(pop_node());
  }

  if (auto end = parse_class_specifier(a, b)) {
    a = end;
    result->append(pop_node());
  }

  result->_keyword = result->child(0);
  result->_params  = result->child(1);
  result->_class   = result->child(2);

  push_node(result);
  return a;
}

//----------------------------------------

const Token* parse_declaration_list(const Token* a, const Token* b, NodeType type, const char ldelim, const char spacer, const char rdelim) {
  if (!a[0].is_punct(ldelim)) return nullptr;

  auto result = new NodeList(type);

  a = skip_punct(a, b, ldelim);

  while(1) {
    if (a->is_punct(rdelim)) {
      a = skip_punct(a, b, rdelim);
      break;
    }
    else if (a->is_punct(spacer)) {
      a = skip_punct(a, b, spacer);
    }
    else {
      if (auto end = parse_declaration(a, b, rdelim)) {
        a = end;
      }
      result->append(pop_node());
    }
  }

  push_node(result);
  return a;
}

//----------------------------------------

const Token* parse_expression_list(const Token* a, const Token* b, NodeType type, const char ldelim, const char spacer, const char rdelim) {
  if (!a[0].is_punct(ldelim)) return nullptr;

  auto result = new NodeList(type);

  a = skip_punct(a, b, ldelim);

  while(1) {
    if (a->is_punct(rdelim)) {
      a = skip_punct(a, b, rdelim);
      break;
    }
    else if (a->is_punct(spacer)) {
      a = skip_punct(a, b, spacer);
    }
    else {
      a = parse_expression(a, b, rdelim);
      result->append(pop_node());
    }
  }

  push_node(result);
  return a;
}

//----------------------------------------

const Token* parse_type_qualifier(const Token* a, const Token* b) {
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
    if (a->is_identifier(keywords[i])) {
      return take_identifier(a, b);
    }
  }

  assert(false);
  return nullptr;
}

//----------------------------------------

const Token* parse_translation_unit(const Token* a, const Token* b) {

  using pattern = Oneof<
    Ref<parse_template_decl>,
    pattern_preproc,
    pattern_external_declaration
  >;

  auto old_top = node_top;
  while(!a->is_eof()) {
    a = pattern::match(a, b);
    assert(a);
  }

  auto result = new TranslationUnit();
  for (auto i = old_top; i < node_top; i++) {
    result->append(node_stack[i]);
  }
  node_top = old_top;

  push_node(result);
  return a;
}
































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

  std::vector<std::string> paths;

  //const char* base_path = "tests";
  //const char* base_path = "mini_tests";
  const char* base_path = argc > 1 ? argv[1] : "tests";


  printf("Parsing source files in %s\n", base_path);
  for (const auto& f : rdit(base_path)) {
    if (!f.is_regular_file()) continue;
    paths.push_back(f.path().native());
  }

  //paths = { "tests/constructor_args.h" };

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

    const Token* token = tokens.data();
    const Token* token_eof = tokens.data() + tokens.size() - 1;

    printf("Parsing %s\n", path.c_str());

    parse_accum -= timestamp_ms();
    parse_translation_unit(token, token_eof);
    parse_accum += timestamp_ms();

    auto root = pop_node();
    //printf("\n");
    //root->dump_tree();
    //printf("\n");
    delete root;

  }

  printf("Lexing took  %f msec\n", lex_accum);
  printf("Parsing took %f msec\n", parse_accum);

  return 0;
}

//------------------------------------------------------------------------------
