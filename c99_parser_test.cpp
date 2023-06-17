#include "c99_parser.h"

#include <filesystem>
#include "Node.h"

#include "Lexemes.h"
#include <vector>
#include <memory.h>

double timestamp_ms();

//==============================================================================

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

const Token* parse_declaration(const Token* a, const Token* b);
const Token* parse_external_declaration(const Token* a, const Token* b);

struct TestPattern : public NodeMaker<TestPattern> {
  using pattern = Some<Ref<parse_external_declaration>>;
};

//------------------------------------------------------------------------------

const Token* parse_function(const Token* a, const Token* b);

int test_c99_peg(int argc, char** argv) {
  printf("Parseroni Demo\n");

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
  //paths = { "../gcc/gcc/testsuite/gcc.c-torture/execute/pr39501.c"};

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
  int file_skip = 0;

  for (const auto& path : paths) {
    text.clear();
    lexemes.clear();
    tokens.clear();

    file_total++;

    if (!path.ends_with(".cpp") &&
        !path.ends_with(".hpp") &&
        !path.ends_with(".c") &&
        !path.ends_with(".h")) continue;

    if (
         path.ends_with("return-addr.c")
      || path.ends_with("complex-6.c")     // requires preproc
      || path.ends_with("loop-13.c")       // requires preproc
      || path.ends_with("va-arg-24.c")     // requires preproc
      //|| path.ends_with("pr39501.c")       // requires preproc
    )
    {
      file_skip++;
      continue;
    }

    {
      auto size = std::filesystem::file_size(path);
      text.resize(size);
      memset(text.data(), 0, size);
      FILE* file = fopen(path.c_str(), "rb");
      if (!file) {
        printf("Could not open %s!\n", path.c_str());
      }
      auto r = fread(text.data(), 1, size, file);
    }

    if (text.find("#define") != std::string::npos) {
      file_skip++;
      continue;
    }

    //printf("Lexing %s\n", path.c_str());

    lex_accum -= timestamp_ms();
    lex_file(text, lexemes, tokens);
    lex_accum += timestamp_ms();

    //dump_lexemes(path, text, lexemes);

    const Token* token_a = tokens.data();
    const Token* token_b = tokens.data() + tokens.size() - 1;

    printf("%04d: Parsing %s...", file_pass, path.c_str());

    parse_accum -= timestamp_ms();
    //parse_function(token_a, token_b);
    NodeTranslationUnit::match(token_a, token_b);
    //TestPattern::match(token_a, token_b);
    parse_accum += timestamp_ms();

    if (NodeBase::node_stack.top() != 1) {
      printf("Parsing %s failed!\n", path.c_str());
      printf("Node stack wrong size %ld\n", NodeBase::node_stack._top);
      return -1;
    }

    auto root = NodeBase::node_stack.pop();

    //root->dump_tree();

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
      printf("OK!\n");
      file_pass++;
    }

    delete root;

  }

  printf("\n");
  printf("Files total %d\n", file_total);
  printf("Files pass  %d\n", file_pass);
  printf("Files fail  %d\n", file_total - file_pass - file_skip);
  printf("Files skip  %d\n", file_skip);
  printf("Lexing took  %f msec\n", lex_accum);
  printf("Parsing took %f msec\n", parse_accum);

  return 0;
}

//------------------------------------------------------------------------------
