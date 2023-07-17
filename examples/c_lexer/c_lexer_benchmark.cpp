// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

#include "examples/c_lexer/CLexer.hpp"
#include "examples/c_lexer/CToken.hpp"

#include <filesystem>
#include <stdint.h>    // for uint8_t
#include <stdio.h>
#include <stdlib.h>    // for exit
#include <string.h>    // for memset
#include <assert.h>
#include <string>
#include <vector>

using namespace matcheroni;

//------------------------------------------------------------------------------

int total_files = 0;
int total_bytes = 0;
int source_files = 0;

std::vector<std::string> bad_files;
std::vector<std::string> skipped_files;

double lex_time = 0;

std::vector<std::string> negative_test_cases = {
  "/objc/",                           // Objective C
  "/objc.dg/",                        // Objective C
  "/objc-obj-c++-shared/",            // Objective C
  "/asm/", // all assembler

  "19990407-1.c",                     // requires preproc
  "c2x-digit-separators-2.c",         // dg-error
  "c2x-utf8char-3.c",                 // dg-error
  "charconst.c",                      // dg-error
  "delimited-escape-seq-3.c",         // Empty delimited escape
  "delimited-escape-seq-4.c",         // Intentionally broken
  "delimited-escape-seq-5.c",         // Intentionally broken
  "delimited-escape-seq-6.c",         // Unterminated escape sequence
  "delimited-escape-seq-7.c",         // Unterminated escape sequence
  "dir-only-1.c",                     // -fdirectives-only
  "dir-only-8.c",                     // -fdirectives-only
  "escape-1.c",                       // dg-error
  "escape-2.c",                       // bad escape sequence
  "lexstrng.c",                       // trigraphs
  "mac-eol-at-eof.c",                 // Text with only \r and not \n?
  "macroargs.c",                      // preproc
  "missing-header-fixit-1.c",         // preproc
  "missing-header-fixit-2.c",         // preproc
  "multiline-2.c",                    // dg-error
  "multiline.c",                      // Not valid C
  "named-universal-char-escape-3.c",  // Empty named escape
  "named-universal-char-escape-5.c",  // Intentionally broken
  "named-universal-char-escape-6.c",  // Empty named escape
  "named-universal-char-escape-7.c",  // Empty named escape
  "normalize-1.c",                    // Not lexable without preproc
  "normalize-2.c",                    // Not lexable without preproc
  "normalize-3.c",                    // Not lexable without preproc
  "normalize-4.c",                    // Not lexable without preproc
  "pr100646-2.c",                     // Backslash at end of file
  "pr25993.c",                        // preproc
  "quote.c",                          // unterminated strings
  "raw-string-10.c",                  // Not lexable without preproc
  "raw-string-12.c",                  // Nulls inside raw strings
  "raw-string-14.c",                  // trigraphs
  "strify2.c",                        // preproc
  "trigraphs.c",                      // trigraphs
  "ucnid-7.c",                        // Intentionally invalid universal char code
  "ucs.c",                            // bad universal char name
  "utf16-4.c",                        // dg-error
  "utf32-4.c",                        // dg-error
  "warn-normalized-1.c",              // Not lexable without preproc
  "warn-normalized-2.c",              // Not lexable without preproc
  "warning-zero-in-literals-1.c",     // Nulls inside literals
  "include2.c", // dg-error
  "directiv.c", // dg-error
  "strify3.c", // x@y?
  "ppc-asm.h", // @notoc?
  "has-include-1.c", // dg-error
  "has-include-next-1.c", // dg-error
  "pragma-message.c", // dg-error
  "macspace1.c", // dg-error
  "macspace2.c", // dg-error
  "literals-2.c", // dg-error
  "recurse-3.c", // recursive macros that don't compile
  "if-2.c", // dg-error
  "abitest.h", // assembler
  "dce110_timing_generator.c", // "@TODOSTEREO" in if'd out block
  "carl9170/phy.h", // invalid backslash in macro
  "linux/elfnote.h", // assembly
  "boot/ppc_asm.h", // assembly
  "openacc_lib.h", // ...fortran?
  "encoding-issues-", // embedded nuls
};

//------------------------------------------------------------------------------

bool test_lex(CLexer& lexer, const std::string& path, size_t size, bool echo) {

  /*
  std::string text;
  text.resize(size + 1);
  memset(text.data(), 0, size + 1);
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) {
    printf("Could not open %s!\n", path.c_str());
  }
  auto r = fread(text.data(), 1, size, f);
  total_bytes += size;
  text[size] = 0;
  fclose(f);
  */
  std::string text;
  read(path.c_str(), text);
  total_bytes += text.size();
  text.push_back(0);

  bool skip = false;
  // Filter out all the header files that are actually assembly
  if (text.find("__ASSEMBLY__") != std::string::npos) skip = true;
  if (text.find("__ASSEMBLER__") != std::string::npos) skip = true;
  if (text.find("@function") != std::string::npos) skip = true;
  if (text.find("@progbits") != std::string::npos) skip = true;
  if (text.find(".macro") != std::string::npos) skip = true;

  if (skip) {
    skipped_files.push_back(path);
    return false;
  }

  source_files++;

  lexer.reset();

  lex_time -= timestamp_ms();
  bool lex_ok = lexer.lex(to_span(text));
  lex_time += timestamp_ms();

  if (!lex_ok) {
    printf("Lexing failed for file %s:\n", path.c_str());
    bad_files.push_back(path);
  }

  return lex_ok;
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Matcheroni Demo\n");

  using rdit = std::filesystem::recursive_directory_iterator;
  const char* base_path = argc > 1 ? argv[1] : ".";

  CLexer lexer;
  bool echo = false;

  printf("Lexing source files in %s\n", base_path);
  auto time_a = timestamp_ms();
  for (const auto& f : rdit(base_path)) {
    if (f.is_regular_file()) {
      total_files++;
      auto& path = f.path().native();
      auto size = f.file_size();

      bool is_source =
        path.ends_with(".c") ||
        path.ends_with(".cpp") ||
        path.ends_with(".h") ||
        path.ends_with(".hpp");
      if (!is_source) continue;

      bool skip = false;
      for (const auto& f : negative_test_cases) {
        if (path.find(f) != std::string::npos) {
          skipped_files.push_back(path);
          skip = true;
          break;
        }
      }
      if (skip) continue;

      test_lex(lexer, path, size, echo);
    }
  }
  auto time_b = timestamp_ms();
  auto total_time = time_b - time_a;

  printf("\n");
  printf("Total time %f msec\n", total_time);
  printf("Lex time %f msec\n", lex_time);
  printf("Total files %d\n", total_files);
  printf("Total source files %d\n", source_files);
  printf("Total bytes %d\n", total_bytes);
  printf("Files skipped %ld\n", skipped_files.size());
  printf("Files with lex errors %ld\n", bad_files.size());
  printf("\n");

  return 0;
}

//------------------------------------------------------------------------------
