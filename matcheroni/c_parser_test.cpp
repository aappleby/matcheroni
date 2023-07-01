#include "c_parser.h"

#include <filesystem>

void dump_tree(const ParseNode* n, int max_depth = 0, int indentation = 0);

//------------------------------------------------------------------------------
// File filters

bool should_skip(const std::string& path) {
  if (!path.ends_with(".c")) return true;

  std::vector<std::string> bad_files = {
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

    // Function parameter name shadows a typedef'd type :P
    "20010114-1.c",

    // Local variable name shadows a typedef'd struct name
    "921109-1.c",

    // Struct field name shadows typedef'd struct name
    "920428-2.c",
    "20010518-1.c",
    "pr44784.c",

    // Double-typedef
    "pr61159.c",
  };

  for (auto b : bad_files) if (path.ends_with(b)) return true;
  return false;
}

//------------------------------------------------------------------------------

int test_parser(int argc, char** argv) {
  printf("Matcheroni c_parser_test\n");

  std::vector<std::string> paths;
  const char* base_path = argc > 1 ? argv[1] : "tests";

  C99Parser parser;

  bool verbose = false;

#if 0
  paths = {
    //"tests/scratch.c",
    "../gcc/gcc/testsuite/gcc.c-torture/compile/pr44784.c",
  };

  verbose = true;

#else

  printf("Parsing all source files in %s\n", base_path);
  using rdit = std::filesystem::recursive_directory_iterator;
  for (const auto& f : rdit(base_path)) {
    if (!f.is_regular_file()) continue;
    auto path = f.path().native();
    if (!should_skip(path)) {
      paths.push_back(path);
    }
    else {
      parser.file_skip++;
    }
  }
#endif

  // Load all the files, filtering out files that use the preprocessor or that
  // use builtin macros like va_arg.

  for (const auto& path : paths) {
    parser.reset();

    //printf("Loading %s\n", path.c_str());
    parser.load(path);

    // va_arg is a macro
    if (parser.text.find("va_arg") != std::string::npos) {
      //printf("Skipping %s\n", path.c_str());
      parser.file_skip++;
      continue;
    }

    //printf("Lexing %s\n", path.c_str());
    parser.lex();
    if (parser.tokens.empty() || parser.tokens.size() == 2) {
      parser.file_skip++;
      continue;
    }

    // Filter all files containing preproc, but not if they're a csmith file
    if (path.find("csmith") == std::string::npos) {
      bool has_preproc = false;
      for (auto t : parser.tokens) {
        if (t.get_type() == LEX_PREPROC) {
          has_preproc = true;
          break;
        }
      }
      if (has_preproc) {
        parser.file_skip++;
        continue;
      }
    }

    /*
    if (verbose) {
      parser.dump_tokens();
      printf("\n");
    }
    */

    printf("%04d: Parsing %s\n", parser.file_pass, path.c_str());
    auto root = parser.parse();

    if (!root) {
      printf("Parsing failed: %s\n", path.c_str());
    }
    else {
      if (verbose) {
        printf("\n");
        printf("Dumping tree:\n");
        dump_tree(root);
        printf("\n");

        //printf("Dumping tokens:\n");
        //parser.dump_tokens();
        //printf("\n");
      }
    }


    auto tok_a = parser.tokens.data();
    auto tok_b = parser.tokens.data() + parser.tokens.size() - 1;

    CHECK(root->tok_a == tok_a + 1);
    CHECK(root->tok_b == tok_b - 1);

#ifdef DEBUG
    root->check_sanity();
#endif
  }


  printf("\n");
  parser.dump_stats();

  return 0;
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  test_parser(argc, argv);
}

//------------------------------------------------------------------------------
