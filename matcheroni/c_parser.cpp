#include "c_parser.h"
#include "c_lexer.h"

#include <filesystem>
#include <vector>
#include <stdint.h>
#include <cstring>

void dump_tree(const ParseNode* n, int max_depth = 0, int indentation = 0);

//------------------------------------------------------------------------------

double timestamp_ms() {
  using clock = std::chrono::high_resolution_clock;
  using nano = std::chrono::nanoseconds;

  static bool init = false;
  static double origin = 0;

  auto now = clock::now().time_since_epoch();
  auto now_nanos = std::chrono::duration_cast<nano>(now).count();
  if (!origin) origin = now_nanos;

  return (now_nanos - origin) * 1.0e-6;
}

//------------------------------------------------------------------------------

void set_color(uint32_t c) {
  if (c) {
    printf("\u001b[38;2;%d;%d;%dm", (c >> 0) & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF);
  }
  else {
    printf("\u001b[0m");
  }
}

//------------------------------------------------------------------------------

C99Parser::C99Parser() {
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

  class_types.clear();
  struct_types.clear();
  union_types.clear();
  enum_types.clear();
  typedef_types.clear();

  cleanup_accum += timestamp_ms();
}

//------------------------------------------------------------------------------

void C99Parser::load(const std::string& path) {
  io_accum -= timestamp_ms();
  auto size = std::filesystem::file_size(path);
  text.resize(size);
  memset(text.data(), 0, size);
  FILE* file = fopen(path.c_str(), "rb");
  if (!file) {
    printf("Could not open %s!\n", path.c_str());
  }
  auto r = fread(text.data(), 1, size, file);
  fclose(file);

  for (auto c : text) if (c == '\n') file_lines++;

  io_accum += timestamp_ms();
  file_bytes += text.size();
}

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
    //if (!l->is_gap() && !l->is_preproc()) {
    if (!l->is_gap()) {
      tokens.push_back(Token(l));
    }
  }

  DCHECK(tokens.back().lex->is_eof());

  lex_accum += timestamp_ms();
}

//------------------------------------------------------------------------------

ParseNode* C99Parser::parse() {

  SpanTranslationUnit* root = nullptr;

  DCHECK(tokens.front().lex->type == LEX_BOF);
  DCHECK(tokens.back().lex->type == LEX_EOF);

  auto tok_a = tokens.data() + 1;
  auto tok_b = tokens.data() + tokens.size() - 1;

  DCHECK(tok_a->lex->type != LEX_BOF);
  DCHECK(tok_b->lex->type == LEX_EOF);

  // Skip over BOF
  auto cursor = tok_a;

  /*
  if (cursor == tok_b) {
    file_pass++;
    return nullptr;
  }
  */

  parse_accum -= timestamp_ms();
  cursor = SpanTranslationUnit::match(this, cursor, tok_b);
  parse_accum += timestamp_ms();

  if (cursor) {
    DCHECK(tok_a->span->is_a<SpanTranslationUnit>());
    root = tok_a->span->as_a<SpanTranslationUnit>();

    if (cursor != tok_b) {
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

  this->root = root;
  return root;
}

//------------------------------------------------------------------------------

Token* C99Parser::match_builtin_type_base(Token* a, Token* b) {
  if (!a || a == b) return nullptr;
  auto result = SST<builtin_type_base>::match(a->lex->span_a, a->lex->span_b);
  return result ? a + 1 : nullptr;
}

Token* C99Parser::match_builtin_type_prefix(Token* a, Token* b) {
  if (!a || a == b) return nullptr;
  auto result = SST<builtin_type_prefix>::match(a->lex->span_a, a->lex->span_b);
  return result ? a + 1 : nullptr;
}

Token* C99Parser::match_builtin_type_suffix(Token* a, Token* b) {
  if (!a || a == b) return nullptr;
  auto result = SST<builtin_type_suffix>::match(a->lex->span_a, a->lex->span_b);
  return result ? a + 1 : nullptr;
}

//------------------------------------------------------------------------------

void C99Parser::dump_stats() {
  double total_time = io_accum + lex_accum + parse_accum + cleanup_accum;

  // 681730869 - 571465032 = Benchmark creates 110M expression wrapper

  if (file_pass == 10000 && ParseNode::constructor_count != 667723766) {
    set_color(0x008080FF);
    printf("############## NODE COUNT MISMATCH\n");
    set_color(0);
  }
  printf("Total time     %f msec\n", total_time);
  printf("IO time        %f msec\n", io_accum);
  printf("Lexing time    %f msec\n", lex_accum);
  printf("Parsing time   %f msec\n", parse_accum);
  printf("Cleanup time   %f msec\n", cleanup_accum);
  printf("\n");
  printf("Bytes/sec      %f\n", 1000.0 * double(file_bytes) / double(total_time));
  printf("Lines/sec      %f\n", 1000.0 * double(file_lines) / double(total_time));
  printf("\n");
  printf("Total nodes    %d\n", ParseNode::constructor_count);
  printf("Node pool      %ld bytes\n", ParseNode::slabs.max_size);
  printf("File pass      %d\n", file_pass);
  printf("File fail      %d\n", file_fail);
  printf("File skip      %d\n", file_skip);
  printf("File bytes     %d\n", file_bytes);
  printf("File lines     %d\n", file_lines);
  printf("Average line   %f bytes\n", double(file_bytes) / double(file_lines));

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

void print_escaped(const char* s, int len, unsigned int color) {
  if (len < 0) {
    exit(1);
  }
  set_color(color);
  while(len) {
    auto c = *s++;
    if      (c == 0)    break;
    else if (c == ' ')  putc(' ', stdout);
    else if (c == '\n') putc(' ', stdout);
    else if (c == '\r') putc(' ', stdout);
    else if (c == '\t') putc(' ', stdout);
    else                putc(c,   stdout);
    len--;
  }
  while(len--) putc('#', stdout);
  set_color(0);

  //for (int i = 32; i < 256; i++) putc(i, stdout);

  return;
}

std::string escape_span(const ParseNode* n) {
  if (!n->tok_a || !n->tok_b) {
    return "<bad span>";
  }

  auto lex_a = n->tok_a->lex;
  auto lex_b = n->tok_b->lex;
  auto len = lex_b->span_b - lex_a->span_a;

  std::string result;
  for (auto i = 0; i < len; i++) {
    auto c = lex_a->span_a[i];
    if (c == '\n') {
      result.push_back('\\');
      result.push_back('n');
    }
    else if (c == '\r') {
      result.push_back('\\');
      result.push_back('r');
    }
    else if (c == '\t') {
      result.push_back('\\');
      result.push_back('t');
    }
    else {
      result.push_back(c);
    }
    if (result.size() >= 80) break;
  }

  return result;
}


void dump_tree(const ParseNode* n, int max_depth, int indentation) {
  if (max_depth && indentation == max_depth) return;

  printf("%p {%p-%p} ", n, n->tok_a, n->tok_b);

  for (int i = 0; i < indentation; i++) printf(" | ");

  if (n->tok_a) set_color(lex_to_color(*(n->tok_a->lex)));
  //if (!field.empty()) printf("%-10.10s : ", field.c_str());

  n->print_class_name(20);
  set_color(0);

  printf(" '%s'\n", escape_span(n).c_str());

  for (auto c = n->head; c; c = c->next) {
    dump_tree(c, max_depth, indentation + 1);
  }
}

//------------------------------------------------------------------------------

void C99Parser::dump_lexemes() {
  for(auto& l : lexemes) {
    printf("{");
    dump_lexeme(l);
    printf("}");
    printf("\n");
  }
}

//------------------------------------------------------------------------------

void C99Parser::dump_tokens() {
  for (auto& t : tokens) {
    dump_token(t);
  }
}

//------------------------------------------------------------------------------
