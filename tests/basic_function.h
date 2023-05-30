#include "metron/tools/metron_tools.h"

// Private const methods should turn into SV functions.

class Module {
public:

  void tock() {
    tick();
  }

private:

  void tick() {
    my_reg = my_reg + some_func();
  }

  logic<8> some_func() const {
    return 3;
  }

  logic<8> my_reg;
};
