#include "metron/tools/metron_tools.h"

// Multiple tick methods are OK as long as they're all called by tock()

class Module {
public:

  logic<8> tock() {
    logic<8> result = my_reg1 + my_reg2;
    tick1();
    tick2();
    return result;
  }

private:

  void tick1() {
    my_reg1 = 0;
  }

  void tick2() {
    my_reg2 = 1;
  }

  logic<8> my_reg1;
  logic<8> my_reg2;
};
