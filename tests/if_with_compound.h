#include "metron/tools/metron_tools.h"

// If statements whose sub-blocks contain submodule calls _must_ use {}.

class Submod {
public:
  void tock(logic<8> arg) {
    tick(arg);
  }
private:
  void tick(logic<8> arg) {
    my_reg = my_reg + arg;
  }

  logic<8> my_reg;
};


class Module {
public:

  void tock() {
    if (1) {
      submod.tock(72);
    }
    else {
      submod.tock(36);
    }
  }

  Submod submod;
};
