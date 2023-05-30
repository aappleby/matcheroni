#include "metron/tools/metron_tools.h"

// DONTCARE gets translated to 'x
// bN(DONTCARE) gets translated to N'bx

class Module {
public:

  logic<8> test1() {
    return DONTCARE;
  }

  logic<8> test2() {
    return b8(DONTCARE);
  }

  logic<8> test3() {
    return bx<8>(DONTCARE);
  }
};
