#include "metron/tools/metron_tools.h"

// UTF-8 text files with a byte order mark should be supported.

// From https://www.w3.org/2001/06/utf-8-test/UTF-8-demo.html:

/*
Runes:

  ᚻᛖ ᚳᚹᚫᚦ ᚦᚫᛏ ᚻᛖ ᛒᚢᛞᛖ ᚩᚾ ᚦᚫᛗ ᛚᚪᚾᛞᛖ ᚾᚩᚱᚦᚹᛖᚪᚱᛞᚢᛗ ᚹᛁᚦ ᚦᚪ ᚹᛖᛥᚫ

  (Old English, which transcribed into Latin reads 'He cwaeth that he
  bude thaem lande northweardum with tha Westsae.' and means 'He said
  that he lived in the northern land near the Western Sea.')
*/

class Module {
public:

  Module() {
    my_reg = 7;
  }

  logic<8> get_reg() {
    return my_reg;
  }

private:

  logic<8> my_reg;
};
