#include "c_parser.h"

#include "ParseNode.h"
#include "Lexemes.h"
#include "c_lexer.h"

#include <filesystem>
#include <vector>
#include <memory.h>

//------------------------------------------------------------------------------

int file_total = 0;
int file_pass = 0;
int file_keep = 0;
int file_bytes = 0;
int file_lines = 0;

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

C99Parser parser;

int test_parser(const std::string& path) {
  bool verbose = false;

  parser.text.reserve(65536);
  parser.lexemes.reserve(65536);
  parser.tokens.reserve(65536);

  //----------------------------------------
  // Read the source file

  parser.load(path);

  for (auto c : parser.text) if (c == '\n') file_lines++;

  //if (text.find("#define") != std::string::npos) {
  //  return -1;
  //}

  file_keep++;
  file_bytes += parser.text.size();

  //----------------------------------------
  // Lex the source file

  //printf("Lexing %s\n", path.c_str());

  parser.lex_accum -= timestamp_ms();
  parser.lex();
  parser.lex_accum += timestamp_ms();

  assert(parser.tokens.back().lex->is_eof());

  //dump_lexemes(lexemes);
  //return 0;

  Token* token_a = parser.tokens.data();
  Token* token_b = parser.tokens.data() + parser.tokens.size() - 1;

  //----------------------------------------
  // Parse the source file

  printf("%04d: Parsing %s\n", file_pass, path.c_str());

  parser.parse_accum -= timestamp_ms();
  parser.parse(token_a, token_b);
  parser.parse_accum += timestamp_ms();

  if (verbose) parser.dump_tokens();

  //----------------------------------------
  // Sanity check parse result

  ParseNode* root = parser.tokens[0].top;
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
  parser.cleanup_accum -= timestamp_ms();
  parser.reset();
  parser.cleanup_accum += timestamp_ms();

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
  printf("IO time        %f msec\n", parser.io_accum);
  printf("Lexing time    %f msec\n", parser.lex_accum);
  printf("Parsing time   %f msec\n", parser.parse_accum);
  printf("Cleanup time   %f msec\n", parser.cleanup_accum);
  printf("Total time     %f msec\n", total_time);
  printf("Overhead time  %f msec\n", total_time - parser.io_accum - parser.lex_accum - parser.parse_accum - parser.cleanup_accum);
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
