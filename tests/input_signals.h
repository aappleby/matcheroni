#include "metron/tools/metron_tools.h"

// Public fields that are read by the module but never written become input
// ports.

class Submod {
public:

  logic<8> i_signal;
  logic<8> o_signal;
  logic<8> o_reg;

  logic<8> tock(logic<8> i_param) {
    o_signal = i_signal + i_param;
    tick();
    return o_signal + 7;
  }

private:

  void tick() {
    o_reg = o_reg + o_signal;
  }
};

class Module {
public:

  void tock() {
    submod.i_signal = 12;
    logic<8> submod_return = submod.tock(13);
    my_sig = submod_return + 3;
    tick();
  }

  logic<8> my_reg;

private:

  void tick() {
    my_reg = my_reg + my_sig - 2;
  }

  logic<8> my_sig;

  Submod submod;
};
