#include "c_parser.h"

#include <filesystem>

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

int test_parser(int argc, char** argv) {
  printf("Matcheroni c_parser_test\n");

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

  C99Parser parser;

  for (const auto& path : paths) {
    parser.file_total++;
    if (should_skip(path)) continue;

    //printf("Loading %s\n", path.c_str());
    parser.load(path);

    //printf("Lexing %s\n", path.c_str());
    parser.lex();

    printf("%04d: Parsing %s\n", parser.file_pass, path.c_str());

    auto root = parser.parse();
    if (!root) {
      printf("Parsing failed: %s\n", path.c_str());
    }

    parser.reset();
  }

#endif


  printf("\n");
  parser.dump_stats();
  printf("text count %d\n", Lexeme::text_count);

  return 0;
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  test_parser(argc, argv);
}

//------------------------------------------------------------------------------
