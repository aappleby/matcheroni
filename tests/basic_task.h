#include "metron/tools/metron_tools.h"

// Private non-const methods should turn into SV tasks.

class Module {
public:

  int tock() {
    tick();
    return 0;
  }

private:

  void tick() {
    my_reg = my_reg + my_reg2 + 3;
    some_task2();
  }

  void some_task2() {
    my_reg2 = my_reg2 + 3;
  }

  logic<8> my_reg;
  logic<8> my_reg2;
};
