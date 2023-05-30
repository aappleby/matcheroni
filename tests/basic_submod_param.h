#include "metron/tools/metron_tools.h"

// We can instantiated templated classes as submodules.

template<int SOME_CONSTANT = 6>
class Submod {
public:

  void tock() {
    tick();
  }

private:

  void tick() {
    sub_reg = sub_reg + SOME_CONSTANT;
  }

  logic<8> sub_reg;
};

class Module {
public:

  void tock() {
    submod.tock();
  }

  Submod<99> submod;
};
