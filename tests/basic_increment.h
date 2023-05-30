#include "metron/tools/metron_tools.h"

// Increment/decrement should be translated into equivalent Verilog, but they
// do _not_ return the old/new value.

class Module {
public:

  void tock() {
    tick();
  }

private:

  void tick() {
    my_reg1++;
    ++my_reg2;
    my_reg3--;
    --my_reg4;
  }

  int my_reg1;
  int my_reg2;
  int my_reg3;
  int my_reg4;
};
