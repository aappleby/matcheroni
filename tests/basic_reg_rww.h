#include "metron/tools/metron_tools.h"

// Writing a register multiple times in the same function is OK.

class Module {
public:

  void tock() {
    tick();
  }

private:

  void tick() {
    logic<1> temp = my_reg;
    my_reg = 0;
    my_reg = 1;
  }

  logic<1> my_reg;
};
