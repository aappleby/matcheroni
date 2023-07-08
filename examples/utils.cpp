// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/utils.hpp"

#include <assert.h>
#include <chrono>
#include <filesystem>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>    // for exit
#include <string>
#include <sys/stat.h>

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

  return;
}

//------------------------------------------------------------------------------

void print_typeid_name(const char* name) {
  int name_len = 0;
  if (sscanf(name, "%d", &name_len)) {
    while((*name >= '0') && (*name <= '9')) name++;
    for (int i = 0; i < name_len; i++) {
      putc(name[i], stdout);
    }
  }
  else {
    printf("%s", name);
  }
}

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

std::string read(const char* path) {
  auto size = std::filesystem::file_size(path);
  std::string buf;
  buf.resize(size);
  FILE* f = fopen(path, "rb");
  auto _ = fread(buf.data(), size, 1, f);
  fclose(f);
  return buf;
}

void read(const char* path, std::string& text) {
  auto size = std::filesystem::file_size(path);
  text.resize(size);
  FILE* f = fopen(path, "rb");
  auto _ = fread(text.data(), size, 1, f);
  fclose(f);
}

void read(const char* path, char*& text_out, int& size_out) {
  //auto size = std::filesystem::file_size(path);
  //text.resize(size);

  struct stat statbuf;
  if (stat(path, &statbuf) != -1) {
    text_out = new char[statbuf.st_size];
    size_out = statbuf.st_size;
    FILE* f = fopen(path, "rb");
    auto _ = fread(text_out, size_out, 1, f);
    fclose(f);
  }
}

//------------------------------------------------------------------------------
