// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include <filesystem>

#include "utils.hpp"

#include "c99/C99Lexer.hpp"
#include "c99/C99Parser.hpp"
#include "c99/ParseNode.hpp"
#include "c99/SlabAlloc.hpp"

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

      // uses va_arg, which is actually a macro
      "pr48767.c",
      "20041214-1.c",
      "pr38151.c",
      "pr43066.c",
      "pr40023.c",
      "20001123-1.c",
      "pr34334.c",
      "20081119-1.c",
      "20071214-1.c",

      // Double-typedef
      "pr61159.c",

      // Empty file
      "920821-1.c",

  };

  for (auto b : bad_files)
    if (path.ends_with(b)) return true;
  return false;
}

//------------------------------------------------------------------------------

int test_parser(int argc, char** argv) {
  printf("Matcheroni c_parser_test\n");

  std::vector<std::string> paths;
  const char* base_path = argc > 1 ? argv[1] : "tests";

  C99Lexer lexer;
  C99Parser parser;

  bool verbose = false;
  double io_time = 0;
  double lex_time = 0;
  double parse_time = 0;
  double cleanup_time = 0;

  int file_pass = 0;
  int file_fail = 0;
  int file_skip = 0;
  int file_bytes = 0;
  int file_lines = 0;

#if 0
  paths = {
    //"tests/scratch.c",
    "../gcc/gcc/testsuite/gcc.c-torture/execute/pr64718.c",
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
    } else {
      file_skip++;
    }
  }
#endif

  // Load all the files, filtering out files that use the preprocessor or that
  // use builtin macros like va_arg.

  std::string text;
  text.reserve(65536);

  for (const auto& path : paths) {
    {
      // printf("Cleaning up\n");
      cleanup_time -= timestamp_ms();
      lexer.reset();
      parser.reset();
      cleanup_time += timestamp_ms();
    }

    {
      // printf("Loading %s\n", path.c_str());
      io_time -= timestamp_ms();

      text.clear();
      read(path.c_str(), text);
      for (auto c : text) if (c == '\n') file_lines++;
      file_bytes += text.size();

      io_time += timestamp_ms();
    }

    {
      // printf("Lexing %s\n", path.c_str());
      lex_time -= timestamp_ms();
      lexer.lex(text);
      lex_time += timestamp_ms();
    }

    // Filter all files containing preproc, but not if they're a csmith file
    if (path.find("csmith") == std::string::npos) {
      bool has_preproc = false;
      for (auto& l : lexer.lexemes) {
        if (l.type == LEX_PREPROC) {
          has_preproc = true;
          break;
        }
      }
      if (has_preproc) {
        file_skip++;
        continue;
      }
    }

    printf("%04d: Parsing %s\n", file_pass, path.c_str());
    parse_time -= timestamp_ms();
    bool parse_ok = parser.parse(lexer.lexemes);
    parse_time += timestamp_ms();

    if (!parse_ok) {
      file_fail++;
      printf("\n");
      parser.dump_tokens();
      if (parser.root) parser.root->dump_tree(0, 0);
      printf("\n");
      printf("\n");
      printf("fail!\n");
      printf("Parsing failed: %s\n", path.c_str());
      exit(1);
    }

    file_pass++;
    if (verbose) {
      printf("\n");
      printf("Dumping tree:\n");
      parser.root->dump_tree(0, 0);
      printf("\n");
    }
  }

  printf("\n");

  double total_time = io_time + lex_time + parse_time + cleanup_time;

  // 681730869 - 571465032 = Benchmark creates 110M expression wrapper

  if (file_pass == 10000 && ParseNode::constructor_count != 520803633) {
    set_color(0x008080FF);
    printf("############## NODE COUNT MISMATCH\n");
    set_color(0);
  }

  printf("Total time     %f msec\n", total_time);
  printf("Total bytes    %d\n", file_bytes);
  printf("Total lines    %d\n", file_lines);
  printf("Bytes/sec      %f\n",
         1000.0 * double(file_bytes) / double(total_time));
  printf("Lines/sec      %f\n",
         1000.0 * double(file_lines) / double(total_time));
  printf("Average line   %f bytes\n", double(file_bytes) / double(file_lines));
  printf("\n");
  printf("IO time        %f msec\n", io_time);
  printf("Lexing time    %f msec\n", lex_time);
  printf("Parsing time   %f msec\n", parse_time);
  printf("Cleanup time   %f msec\n", cleanup_time);
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
  } else {
    set_color(0x0080FF80);
    printf("##################\n");
    printf("##     PASS     ##\n");
    printf("##################\n");
    set_color(0);
  }

  return 0;
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) { test_parser(argc, argv); }

//------------------------------------------------------------------------------
