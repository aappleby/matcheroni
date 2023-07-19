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
#include <string>
#include <vector>

using namespace matcheroni;

//------------------------------------------------------------------------------

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
  "pr57824.c", // raw multiline string inside a #pragma
  "raw-string-directive-2.c", // raw multiline string inside a #pragma
  "raw-string-directive-1.c", // raw multiline string inside a #pragma
};

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Matcheroni C Lexer Benchmark\n");

  const char* base_path = argc > 1 ? argv[1] : ".";

  auto time_a = timestamp_ms();

  //----------------------------------------
  // Scan the directory and prune it down to only valid C source files.

  size_t total_lines = 0;
  std::vector<std::string> source_files;
  std::vector<std::string> failed_files;
  std::vector<std::string> bad_files;
  std::vector<std::string> skipped_files;

  printf("Scanning source files in %s\n", base_path);
  using rdit = std::filesystem::recursive_directory_iterator;
  for (const auto& f : rdit(base_path)) {
    if (!f.is_regular_file()) continue;
    auto& path = f.path().native();

    // Filter out all non-C files
    bool is_source =
      path.ends_with(".c") ||
      path.ends_with(".cpp") ||
      path.ends_with(".h") ||
      path.ends_with(".hpp");
    if (!is_source) {
      skipped_files.push_back(path);
      continue;
    }

    // Filter out all the files on our known-bad list
    bool skip = false;
    for (const auto& f : negative_test_cases) {
      if (path.find(f) != std::string::npos) {
        skip = true;
        break;
      }
    }

    if (skip) {
      //printf("Known bad file - %s\n", path.c_str());
      bad_files.push_back(path);
      continue;
    }

    std::string text;
    read(path.c_str(), text);

    // Filter out all the files that are actually assembly
    // We split the string constants so we don't mark _this_ source file as
    // unlexable :D
    if (text.find("__ASSE" "MBLY__") != std::string::npos) skip = true;
    if (text.find("__ASSE" "MBLER__") != std::string::npos) skip = true;
    if (text.find("@fun" "ction") != std::string::npos) skip = true;
    if (text.find("@prog" "bits") != std::string::npos) skip = true;
    if (text.find(".ma" "cro") != std::string::npos) skip = true;

    if (skip) {
      //printf("File contains assembly - %s\n", path.c_str());
      bad_files.push_back(path);
      continue;
    }

    for (auto c : text) if (c == '\n') total_lines++;
    source_files.push_back(f.path());

    //if (source_files.size() == 100) break;
  }

  //----------------------------------------
  // Lex all the good files

  //source_files = {
  //  "./examples/c_lexer/c_lexer_test.cpp"
  //};

  CLexer lexer;
  std::string text;
  size_t total_bytes = 0;
  double lex_msec = 0;
  bool any_fail = false;
  int count = 0;

  printf("\n");
  printf("Lexing %ld source files in %s\n", source_files.size(), base_path);
  for (const auto& path : source_files) {
    text.clear();
    lexer.reset();

    //printf("%05d: Lexing %s\n", count++, path.c_str());

    read(path.c_str(), text);
    total_bytes += text.size();

    lex_msec -= timestamp_ms();
    bool lex_ok = lexer.lex(to_span(text));
    lex_msec += timestamp_ms();
    if (!lex_ok) {
      failed_files.push_back(path);
      printf("Lexing failed for file %s:\n", path.c_str());
    }
  }

  //----------------------------------------
  // Report stats

  auto time_b = timestamp_ms();
  auto total_time = time_b - time_a;

  auto lex_sec = lex_msec / 1000;

  printf("\n");
  printf("Total time  %f msec\n", total_time);
  printf("Lex time    %f msec\n", lex_msec);
  printf("Total files %ld\n", source_files.size());
  printf("Total lines %ld\n", total_lines);
  printf("Total bytes %ld\n", total_bytes);
  printf("File rate   %.2f Kfiles/sec\n",  (source_files.size() / 1e3) / lex_sec);
  printf("Line rate   %.2f Mlines/sec\n",  (total_lines / 1e6) / lex_sec);
  printf("Byte rate   %.2f MBytes/sec\n",  (total_bytes / 1e6) / lex_sec);
  printf("Total failures        %ld\n", failed_files.size());
  printf("Total known-bad files %ld\n", bad_files.size());
  printf("Total skipped files   %ld\n", skipped_files.size());
  printf("\n");

  return failed_files.size() ? -1 : 0;
}

//------------------------------------------------------------------------------
