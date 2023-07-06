#include "c_parser.hpp"

#include "ParseNode.hpp"
#include "SlabAlloc.hpp"
#include "TypeScope.hpp"
#include "utils.hpp"

//------------------------------------------------------------------------------

C99Parser::C99Parser() {
  type_scope = new TypeScope();
  text.reserve(65536);
  lexemes.reserve(65536);
  tokens.reserve(65536);
}

//------------------------------------------------------------------------------

void C99Parser::reset() {
  cleanup_accum -= timestamp_ms();

  text.clear();
  lexemes.clear();
  tokens.clear();
  ParseNode::slabs.reset();

  while (type_scope->parent) pop_scope();
  type_scope->clear();

  cleanup_accum += timestamp_ms();
}

//------------------------------------------------------------------------------

void C99Parser::load(const std::string& path) {
  io_accum -= timestamp_ms();

  /*
  auto size = std::filesystem::file_size(path);
  text.resize(size);
  memset(text.data(), 0, size);
  FILE* file = fopen(path.c_str(), "rb");
  if (!file) {
    printf("Could not open %s!\n", path.c_str());
  }
  auto r = fread(text.data(), 1, size, file);
  fclose(file);
  */
  text = read(path.c_str());

  for (auto c : text)
    if (c == '\n') file_lines++;

  io_accum += timestamp_ms();
  file_bytes += text.size();
}

#if 0

//------------------------------------------------------------------------------

void C99Parser::lex() {
  lex_accum -= timestamp_ms();

  auto text_a = text.data();
  auto text_b = text_a + text.size();

  lexemes.push_back(Lexeme(LEX_BOF, nullptr, nullptr));

  const char* cursor = text_a;
  while(cursor) {
    auto lex = next_lexeme(nullptr, cursor, text_b);
    lexemes.push_back(lex);
    if (lex.type == LEX_EOF) {
      break;
    }
    cursor = lex.span_b;
  }

  for (auto i = 0; i < lexemes.size(); i++) {
    auto l = &lexemes[i];
    if (!l->is_gap()) {
      tokens.push_back(Token(l));
    }
  }

  //CHECK(atom_cmp(&tokens.back(), LEX_EOF) == 0);

  lex_accum += timestamp_ms();
}

//------------------------------------------------------------------------------

ParseNode* C99Parser::parse() {

  NodeTranslationUnit* root = nullptr;

  //CHECK(atom_cmp(&tokens.front(), LEX_BOF) == 0);
  //CHECK(atom_cmp(&tokens.back(), LEX_EOF) == 0);

  // Skip over BOF, stop before EOF
  auto tok_a = tokens.data() + 1;
  auto tok_b = tokens.data() + tokens.size() - 1;

  //CHECK(atom_cmp(tok_a, LEX_BOF) != 0);
  //CHECK(atom_cmp(tok_b, LEX_EOF) == 0);

  parse_accum -= timestamp_ms();
  global_cursor = tok_a;
  auto end = NodeTranslationUnit::match(this, tok_a, tok_b);
  parse_accum += timestamp_ms();

  if (end) {
    CHECK(tok_a->get_span()->is_a<NodeTranslationUnit>());
    root = tok_a->get_span()->as_a<NodeTranslationUnit>();

    if (end != tok_b) {
      file_fail++;
      printf("\n");
      dump_tokens();
      printf("\n");
      dump_tree(root);
      printf("\n");
      printf("fail!\n");
      exit(1);
    }
    else {
      file_pass++;
    }
  }

  this->head = root;
  this->tail = root;
  return root;
}


  int atom_cmp(Token* a, const LexemeType& b) {
    DCHECK(a == global_cursor);
    auto result = a->atom_cmp(b);
    if (result == 0) global_cursor++;
    return result;
  }

  int atom_cmp(Token* a, const char& b) {
    DCHECK(a == global_cursor);
    auto result = a->atom_cmp(b);
    if (result == 0) global_cursor++;
    return result;
  }

  int atom_cmp(Token* a, const char* b) {
    DCHECK(a == global_cursor);
    auto result = a->atom_cmp(b);
    if (result == 0) global_cursor++;
    return result;
  }

  template<int N>
  int atom_cmp(Token* a, const StringParam<N>& b) {
    DCHECK(a == global_cursor);
    auto result = a->atom_cmp(b);
    if (result == 0) global_cursor++;
    return result;
  }

  int atom_cmp(Token* a, const Token* b) {
    DCHECK(a == global_cursor);
    auto result = a->atom_cmp(b);
    if (result == 0) global_cursor++;
    return result;
  }

  inline static int rewind_count = 0;
  inline static int didnt_rewind = 0;

  void atom_rewind(Token* a, Token* b) {
    //printf("rewind to %20.20s\n", a->debug_span_a());

    /*
    if (a < global_cursor) {
      static constexpr int context_len = 60;
      printf("[");
      print_escaped(global_cursor->get_lex_debug()->span_a, context_len, 0x804080);
      printf("]\n");
      printf("[");
      print_escaped(a->get_lex_debug()->span_a, context_len, 0x804040);
      printf("]\n");
    }
    */

    DCHECK(a <= global_cursor);

    if (a < global_cursor) {
      rewind_count++;
    }
    else {
      didnt_rewind++;
    }

    global_cursor = a;
  }

inline int atom_cmp(void* ctx, Token* a, LexemeType b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

inline int atom_cmp(void* ctx, Token* a, char b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

inline int atom_cmp(void* ctx, Token* a, const char* b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

template<int N>
inline int atom_cmp(void* ctx, Token* a, const StringParam<N>& b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

inline int atom_cmp(void* ctx, Token* a, const Token* b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}

inline void atom_rewind(void* ctx, Token* a, Token* b) {
  ((C99Parser*)ctx)->atom_rewind(a, b);
}

//------------------------------------------------------------------------------

Token* C99Parser::match_class_type  (Token* a, Token* b) { return type_scope->has_class_type  (this, a, b) ? a + 1 : nullptr; }
Token* C99Parser::match_struct_type (Token* a, Token* b) { return type_scope->has_struct_type (this, a, b) ? a + 1 : nullptr; }
Token* C99Parser::match_union_type  (Token* a, Token* b) { return type_scope->has_union_type  (this, a, b) ? a + 1 : nullptr; }
Token* C99Parser::match_enum_type   (Token* a, Token* b) { return type_scope->has_enum_type   (this, a, b) ? a + 1 : nullptr; }
Token* C99Parser::match_typedef_type(Token* a, Token* b) { return type_scope->has_typedef_type(this, a, b) ? a + 1 : nullptr; }

void C99Parser::add_class_type  (Token* a) { type_scope->add_class_type  (a); }
void C99Parser::add_struct_type (Token* a) { type_scope->add_struct_type (a); }
void C99Parser::add_union_type  (Token* a) { type_scope->add_union_type  (a); }
void C99Parser::add_enum_type   (Token* a) { type_scope->add_enum_type   (a); }
void C99Parser::add_typedef_type(Token* a) { type_scope->add_typedef_type(a); }

//----------------------------------------------------------------------------

void C99Parser::push_scope() {
  TypeScope* new_scope = new TypeScope();
  new_scope->parent = type_scope;
  type_scope = new_scope;
}

void C99Parser::pop_scope() {
  TypeScope* old_scope = type_scope->parent;
  if (old_scope) {
    delete type_scope;
    type_scope = old_scope;
  }
}

//----------------------------------------------------------------------------

void C99Parser::append_node(ParseNode* node) {
  if (tail) {
    tail->next = node;
    node->prev = tail;
    tail = node;
  }
  else {
    head = node;
    tail = node;
  }
}

void C99Parser::enclose_nodes(ParseNode* start, ParseNode* node) {
  // Is this right? Who knows. :D
  node->head = start;
  node->tail = tail;

  tail->next = node;
  start->prev = nullptr;

  tail = node;
}

//------------------------------------------------------------------------------

Token* C99Parser::match_builtin_type_base(Token* a, Token* b) {
  if (!a || a == b) return nullptr;
  auto result = SST<builtin_type_base>::contains(this, a);
  if (result) {
    return a + 1;
  }
  else {
    return nullptr;
  }
}

Token* C99Parser::match_builtin_type_prefix(Token* a, Token* b) {
  if (!a || a == b) return nullptr;
  auto result = SST<builtin_type_prefix>::contains(this, a);
  if (result) {
    return a + 1;
  }
  else {
    return nullptr;
  }
}

Token* C99Parser::match_builtin_type_suffix(Token* a, Token* b) {
  if (!a || a == b) return nullptr;
  auto result = SST<builtin_type_suffix>::contains(this, a);
  if (result) {
    return a + 1;
  }
  else {
    return nullptr;
  }
}

//------------------------------------------------------------------------------

void C99Parser::dump_stats() {
  double total_time = io_accum + lex_accum + parse_accum + cleanup_accum;

  // 681730869 - 571465032 = Benchmark creates 110M expression wrapper

  if (file_pass == 10000 && ParseNode::constructor_count != 511757833) {
    set_color(0x008080FF);
    printf("############## NODE COUNT MISMATCH\n");
    set_color(0);
  }
  printf("Total time     %f msec\n", total_time);
  printf("Total bytes    %d\n", file_bytes);
  printf("Total lines    %d\n", file_lines);
  printf("Bytes/sec      %f\n", 1000.0 * double(file_bytes) / double(total_time));
  printf("Lines/sec      %f\n", 1000.0 * double(file_lines) / double(total_time));
  printf("Average line   %f bytes\n", double(file_bytes) / double(file_lines));
  printf("\n");
  printf("IO time        %f msec\n", io_accum);
  printf("Lexing time    %f msec\n", lex_accum);
  printf("Parsing time   %f msec\n", parse_accum);
  printf("Cleanup time   %f msec\n", cleanup_accum);
  printf("\n");
  printf("Total nodes    %d\n", ParseNode::constructor_count);
  printf("Node pool      %ld bytes\n", ParseNode::slabs.max_size);
  printf("Rewind count   %d\n", C99Parser::rewind_count);
  printf("Didn't rewind  %d\n", C99Parser::didnt_rewind);
  printf("File pass      %d\n", file_pass);
  printf("File fail      %d\n", file_fail);
  printf("File skip      %d\n", file_skip);

  if (file_fail) {
    set_color(0x008080FF);
    printf("##################\n");
    printf("##     FAIL     ##\n");
    printf("##################\n");
    set_color(0);
  }
  else {
    set_color(0x0080FF80);
    printf("##################\n");
    printf("##     PASS     ##\n");
    printf("##################\n");
    set_color(0);
  }
}

//------------------------------------------------------------------------------

void C99Parser::dump_lexemes() {
  for(auto& l : lexemes) {
    printf("{");
    l.dump_lexeme();
    printf("}");
    printf("\n");
  }
}

//------------------------------------------------------------------------------

void C99Parser::dump_tokens() {
  for (auto& t : tokens) {
    t.dump_token();
  }
}

//------------------------------------------------------------------------------
#endif
