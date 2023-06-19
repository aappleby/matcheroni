#include "c_parser.h"

#include <filesystem>
#include "ParseNode.h"

#include "Lexemes.h"
#include <vector>
#include <memory.h>

double timestamp_ms();
void set_color(uint32_t c);

//------------------------------------------------------------------------------

void lex_file(std::string& text, std::vector<Lexeme>& lexemes, std::vector<Token>& tokens) {

  auto text_a = text.data();
  auto text_b = text_a + text.size();

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
    if (!l->is_gap() && !l->is_preproc()) {
      tokens.push_back(Token(lex_to_tok(l->type), l));
    }
  }
}

//------------------------------------------------------------------------------

void dump_lexemes(std::string& text, std::vector<Lexeme>& lexemes) {
  for(auto& l : lexemes) {
    printf("%-15s ", l.str());

    int len = l.span_b - l.span_a;
    if (len > 80) len = 80;

    for (int i = 0; i < len; i++) {
      auto text_a = text.data();
      auto text_b = text_a + text.size();
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

int test_c_peg(int argc, char** argv) {

  std::vector<std::string> paths;
  const char* base_path = argc > 1 ? argv[1] : "tests";

  //base_path = "mini_tests";

  printf("Parsing source files in %s\n", base_path);
  using rdit = std::filesystem::recursive_directory_iterator;
  for (const auto& f : rdit(base_path)) {
    if (!f.is_regular_file()) continue;
    paths.push_back(f.path().native());
  }

  //paths = { "tests/scratch.h" };
  //paths = { "tests/basic_inputs.h" };
  //paths = { "mini_tests/csmith_5.cpp" };
  //paths = { "../gcc/gcc/tree-inline.h" };
  //paths = { "../gcc/gcc/testsuite/gcc.c-torture/execute/921110-1.c"};

  double io_accum = 0;
  double lex_accum = 0;
  double parse_accum = 0;

  std::string text;
  std::vector<Lexeme> lexemes;
  std::vector<Token> tokens;

  text.reserve(65536);
  lexemes.reserve(65536);
  tokens.reserve(65536);

  int file_total = 0;
  int file_pass = 0;
  int file_keep = 0;
  int file_bytes = 0;

  for (const auto& path : paths) {
    text.clear();
    lexemes.clear();
    tokens.clear();
    // Don't forget to reset the parser state derrrrrp
    ParseNode::node_stack.clear_to(0);
    ParseNode::reset_types();

    file_total++;

    //----------
    // File filters

    if (!path.ends_with(".cpp") &&
        !path.ends_with(".hpp") &&
        !path.ends_with(".c") &&
        !path.ends_with(".h")) continue;

    if (path.ends_with("pr56982.c")) continue;     // requires jmp_buf
    if (path.ends_with("20210505-1.c")) continue;  // requires jmp_buf
    if (path.ends_with("sprintf-chk.c")) continue; // requires #include "chk.h"

    // Struct/union declarations in weird places
    if (path.ends_with("20020210-1.c")) continue;
    if (path.ends_with("920928-4.c")) continue;
    if (path.ends_with("pr46866.c")) continue;
    if (path.ends_with("20011217-2.c")) continue;
    if (path.ends_with("pr77754-6.c")) continue;
    if (path.ends_with("pr39394.c")) continue;
    if (path.ends_with("pr85401.c")) continue;

    // Weird stuff inside a switch
    if (path.ends_with("20000105-1.c")) continue;
    if (path.ends_with("pr21356.c")) continue;
    if (path.ends_with("941014-3.c")) continue;
    if (path.ends_with("pr87468.c")) continue;
    if (path.ends_with("20030902-1.c")) continue;
    if (path.ends_with("medce-1.c")) continue;
    if (path.ends_with("pr65241.c")) continue;
    if (path.ends_with("pr34091.c")) continue;

    // __builtin_types_compatible_p (int[5], int[])
    if (path.ends_with("builtin-types-compatible-p.c")) continue;

    // Function "zloop" missing return type so it assumes that the previous
    // struct declaration that is missing a semicolon is the return type :P
    if (path.ends_with("900116-1.c")) continue;

    // Weird type stuff idunno i'm tired
    if (path.ends_with("pr43635.c")) continue;

    // Requires preproc
    if (path.ends_with("structs.c")) continue;

    //----------

    io_accum -= timestamp_ms();
    auto size = std::filesystem::file_size(path);
    text.resize(size);
    memset(text.data(), 0, size);
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) {
      printf("Could not open %s!\n", path.c_str());
    }
    auto r = fread(text.data(), 1, size, file);
    io_accum += timestamp_ms();

    if (text.find("#define") != std::string::npos) {
      continue;
    }

    file_keep++;
    file_bytes += size;

    //printf("Lexing %s\n", path.c_str());

    lex_accum -= timestamp_ms();
    lex_file(text, lexemes, tokens);
    lex_accum += timestamp_ms();

    //dump_lexemes(path, text, lexemes);

    const Token* token_a = tokens.data();
    const Token* token_b = tokens.data() + tokens.size() - 1;

    //printf("%04d: Parsing %s\n", file_pass, path.c_str());

    parse_accum -= timestamp_ms();
    NodeTranslationUnit::match(token_a, token_b);
    parse_accum += timestamp_ms();

    if (ParseNode::node_stack.top() != 1) {
      printf("Parsing failed: %s\n", path.c_str());
      printf("Node stack wrong size %ld\n", ParseNode::node_stack._top);
      return -1;
    }

    auto root = ParseNode::node_stack.pop();
    //root->dump_tree();

    if (root->tok_a != token_a || root->tok_b != token_b) {
      printf("Parsing failed: %s\n", path.c_str());
      //root->dump_tree();
      //break;
    }
    else {
      file_pass++;
    }

    delete root;
  }

  printf("\n");
  printf("IO took      %7.2f msec\n", io_accum);
  printf("Lexing took  %7.2f msec\n", lex_accum);
  printf("Parsing took %7.2f msec\n", parse_accum);
  printf("\n");
  printf("Files total    %d\n", file_total);
  printf("Files filtered %d\n", file_total - file_keep);
  printf("Files kept     %d\n", file_keep);
  printf("Files bytes    %d\n", file_bytes);
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
  printf("\n");

  return 0;
}

//------------------------------------------------------------------------------