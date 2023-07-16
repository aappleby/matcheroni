// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "examples/c_parser/CToken.hpp"
#include "examples/c_parser/CLexeme.hpp"

//----------------------------------------------------------------------------

CToken::CToken(const CLexeme* lex) {
  this->lex = lex;
}

//------------------------------------------------------------------------------
