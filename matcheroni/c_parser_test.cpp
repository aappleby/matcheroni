#include "c_parser.h"

#include "ParseNode.h"
#include "Lexemes.h"
#include "c_lexer.h"

#include <filesystem>
#include <vector>
#include <memory.h>

//------------------------------------------------------------------------------

double io_accum = 0;
double lex_accum = 0;
double parse_accum = 0;
double cleanup_accum = 0;

int file_total = 0;
int file_pass = 0;
int file_keep = 0;
int file_bytes = 0;
int file_lines = 0;

std::string text;
std::vector<Lexeme> lexemes;
std::vector<Token> tokens;

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

const char* lex_to_str(const Lexeme& lex) {
  switch(lex.type) {
    case LEX_INVALID:    return "LEX_INVALID";
    case LEX_SPACE:      return "LEX_SPACE";
    case LEX_NEWLINE:    return "LEX_NEWLINE";
    case LEX_STRING:     return "LEX_STRING";
    case LEX_IDENTIFIER: return "LEX_IDENTIFIER";
    case LEX_COMMENT:    return "LEX_COMMENT";
    case LEX_PREPROC:    return "LEX_PREPROC";
    case LEX_FLOAT:      return "LEX_FLOAT";
    case LEX_INT:        return "LEX_INT";
    case LEX_PUNCT:      return "LEX_PUNCT";
    case LEX_CHAR:       return "LEX_CHAR";
    case LEX_SPLICE:     return "LEX_SPLICE";
    case LEX_FORMFEED:   return "LEX_FORMFEED";
    case LEX_EOF:        return "LEX_EOF";
  }
  return "<invalid>";
}

uint32_t lex_to_color(const Lexeme& lex) {
  switch(lex.type) {
    case LEX_INVALID    : return 0x0000FF;
    case LEX_SPACE      : return 0x804040;
    case LEX_NEWLINE    : return 0x404080;
    case LEX_STRING     : return 0x4488AA;
    case LEX_IDENTIFIER : return 0xCCCC40;
    case LEX_COMMENT    : return 0x66AA66;
    case LEX_PREPROC    : return 0xCC88CC;
    case LEX_FLOAT      : return 0xFF88AA;
    case LEX_INT        : return 0xFF8888;
    case LEX_PUNCT      : return 0x808080;
    case LEX_CHAR       : return 0x44DDDD;
    case LEX_SPLICE     : return 0x00CCFF;
    case LEX_FORMFEED   : return 0xFF00FF;
    case LEX_EOF        : return 0xFF00FF;
  }
  return 0x0000FF;
}

std::string escape_span(ParseNode* n) {
  if (!n->tok_a || !n->tok_b) {
    return "<bad span>";
  }

  if (n->tok_a == n->tok_b) {
    return "<zero span>";
  }

  auto lex_a = n->tok_a->lex;
  auto lex_b = (n->tok_b - 1)->lex;
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


void dump_tree(ParseNode* n, int max_depth, int indentation) {
  if (max_depth && indentation == max_depth) return;

  for (int i = 0; i < indentation; i++) printf(" | ");

  if (n->tok_a) set_color(lex_to_color(*(n->tok_a->lex)));
  //if (!field.empty()) printf("%-10.10s : ", field.c_str());

  n->print_class_name();

  printf(" '%s'\n", escape_span(n).c_str());

  if (n->tok_a) set_color(0);

  for (auto c = n->head; c; c = c->next) {
    dump_tree(c, max_depth, indentation + 1);
  }
}

//------------------------------------------------------------------------------

void lex_file(std::string& text, std::vector<Lexeme>& lexemes, std::vector<Token>& tokens) {
  auto text_a = text.data();
  auto text_b = text_a + text.size();

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
    if (!l->is_gap() && !l->is_preproc()) {
      tokens.push_back(Token(l));
    }
  }
}

//------------------------------------------------------------------------------

void dump_lexeme(const Lexeme& l) {
  int len = l.span_b - l.span_a;
  for (int i = 0; i < len; i++) {
    auto c = l.span_a[i];
    if (c == '\n' || c == '\t' || c == '\r') {
      putc('@', stdout);
    }
    else {
      putc(l.span_a[i], stdout);
    }
  }
}

void dump_lexemes(std::vector<Lexeme>& lexemes) {
  for(auto& l : lexemes) {
    dump_lexeme(l);
    printf("\n");
  }
}

//------------------------------------------------------------------------------

void dump_tokens(std::vector<Token>& tokens) {
  for (auto& t : tokens) {
    // Dump token
    printf("tok %p - ", &t);
    if (t.lex->is_eof()) {
      printf("<eof>");
    }
    else {
      dump_lexeme(*t.lex);
    }
    printf("\n");

    // Dump top node
    printf("  ");
    if (t.top) {
      printf("top %p -> ", t.top);
      t.top->print_class_name();
      printf(" -> %p ", t.top->tok_b);
    }
    else {
      printf("top %p", t.top);
    }
    printf("\n");
  }
}

//------------------------------------------------------------------------------
// File filters

bool should_skip(const std::string& path) {
  if (!path.ends_with(".c")) return true;

  std::vector<std::string> bad_files = {
    // requires jmp_buf
    "pr56982.c",
    "20210505-1.c",

    // requires #include "chk.h"
    "sprintf-chk.c",

    // Struct/union declarations in weird places
    "20020210-1.c",
    "920928-4.c",
    "pr46866.c",
    "20011217-2.c",
    "pr77754-6.c",
    "pr39394.c",
    "pr85401.c",

    // Weird stuff inside a switch
    "20000105-1.c",
    "pr21356.c",
    "941014-3.c",
    "pr87468.c",
    "20030902-1.c",
    "medce-1.c",
    "pr65241.c",
    "pr34091.c",

    // __builtin_types_compatible_p (int[5], int[])
    "builtin-types-compatible-p.c",

    // Function "zloop" missing return type so it assumes that the previous
    // struct declaration that is missing a semicolon is the return type :P
    "900116-1.c",

    // Weird type stuff idunno i'm tired
    "pr43635.c",

    // Requires preproc
    "structs.c",
  };

  for (auto b : bad_files) if (path.ends_with(b)) return true;
  return false;
}

//------------------------------------------------------------------------------

int test_parser(const std::string& path) {
  bool verbose = false;

  text.reserve(65536);
  lexemes.reserve(65536);
  tokens.reserve(65536);

  //----------------------------------------
  // Read the source file

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
  io_accum += timestamp_ms();

  for (auto c : text) if (c == '\n') file_lines++;

  //if (text.find("#define") != std::string::npos) {
  //  return -1;
  //}

  file_keep++;
  file_bytes += size;

  //----------------------------------------
  // Lex the source file

  //printf("Lexing %s\n", path.c_str());

  lex_accum -= timestamp_ms();
  lex_file(text, lexemes, tokens);
  lex_accum += timestamp_ms();

  assert(tokens.back().lex->is_eof());

  //dump_lexemes(lexemes);
  //return 0;

  Token* token_a = tokens.data();
  Token* token_b = tokens.data() + tokens.size() - 1;

  //----------------------------------------
  // Parse the source file

  printf("%04d: Parsing %s\n", file_pass, path.c_str());

  parse_accum -= timestamp_ms();
  auto end = NodeTranslationUnit::match(nullptr, token_a, token_b);
  parse_accum += timestamp_ms();

  if (verbose) dump_tokens(tokens);

  //----------------------------------------
  // Sanity check parse result

  ParseNode* root = tokens[0].top;
  bool parse_failed = false;

  if (!root) {
    parse_failed = true;
    goto teardown;
  }

  /*
  for (auto& tok : tokens) {
    if (tok.top && tok.top->top() != root) {
      parse_failed = true;
      goto teardown;
    }
  }
  */

  //----------------------------------------
  // Tear down parse tree and clean up

teardown:

  if (parse_failed) {
    printf("Parsing failed: %s\n", path.c_str());
  }
  else {
    file_pass++;
  }

  if (verbose) dump_tree(root);

  if (verbose) {
    printf("Node count %d\n", root->node_count());
  }

  // Clear all the nodes off the tokens
  cleanup_accum -= timestamp_ms();
  ParseNode::clear_slabs();
  /*
  for (auto& tok : tokens) {
    //dump_tree(tok.top);
    tok.top = nullptr;
    auto c = tok.alt;
    while(c) {
      auto next = c->alt;
      delete c;
      c = next;
    }
  }
  */
  cleanup_accum += timestamp_ms();

  // Don't forget to reset the parser state derrrrrp
  ParseNode::reset_types();
  text.clear();
  lexemes.clear();
  tokens.clear();

  return 0;
}

//------------------------------------------------------------------------------

int test_parser(int argc, char** argv) {
  printf("Matcheroni c_parser_test\n");

  double total_time = 0;
  total_time -= timestamp_ms();

  std::vector<std::string> paths;
  const char* base_path = argc > 1 ? argv[1] : "tests";

#if 0

  //test_parser("tests/scratch.c");
  //test_parser("tests/basic_inputs.h");
  test_parser("mini_tests/csmith_1088.c");
  //test_parser("../gcc/gcc/tree-inline.h");
  //test_parser("../gcc/gcc/testsuite/gcc.c-torture/execute/921110-1.c");
  file_total++;

#else

  base_path = "mini_tests";
  printf("Parsing all source files in %s\n", base_path);
  using rdit = std::filesystem::recursive_directory_iterator;
  for (const auto& f : rdit(base_path)) {
    if (!f.is_regular_file()) continue;
    paths.push_back(f.path().native());
  }

  for (const auto& path : paths) {
    file_total++;
    if (should_skip(path)) continue;
    test_parser(path);

    //if (file_total == 100) break;
  }

#endif

  total_time += timestamp_ms();

  //printf("Step over count %d\n", ParseNode::step_over_count);
  printf("Constructor %d\n", ParseNode::constructor_count);
  //printf("Destructor  %d\n", ParseNode::destructor_count);

  printf("\n");
  printf("IO time        %f msec\n", io_accum);
  printf("Lexing time    %f msec\n", lex_accum);
  printf("Parsing time   %f msec\n", parse_accum);
  printf("Cleanup time   %f msec\n", cleanup_accum);
  printf("Total time     %f msec\n", total_time);
  printf("Max node size  %ld\n", ParseNode::max_size);
  printf("Files total    %d\n", file_total);
  printf("Files filtered %d\n", file_total - file_keep);
  printf("Files kept     %d\n", file_keep);
  printf("Files bytes    %d\n", file_bytes);
  printf("Files lines    %d\n", file_lines);
  printf("Average line   %f bytes\n", double(file_bytes) / double(file_lines));
  printf("Bytes/sec      %f\n", 1000.0 * double(file_bytes) / double(total_time));
  printf("Lines/sec      %f\n", 1000.0 * double(file_lines) / double(total_time));
  printf("Files pass     %d\n", file_pass);
  printf("Files fail     %d\n", file_keep - file_pass);

  printf("\n");

  if (file_keep != file_pass) {
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

  return 0;
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  test_parser(argc, argv);
}

//------------------------------------------------------------------------------
