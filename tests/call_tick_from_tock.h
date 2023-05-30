#include "metron/tools/metron_tools.h"

// Calling tick() from tock() in the same module should work and should
// generate bindings for the tick() call.

class Module {
public:

  void tock(logic<8> val) {
    tick(val);
  }

private:

  void tick(logic<8> val) {
    my_reg = my_reg + val;
  }

  logic<8> my_reg;

};
