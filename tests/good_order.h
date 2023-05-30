#include "metron/tools/metron_tools.h"

// Declaration order _matters_ - a tock() that reads a reg before the tick()
// that writes it is OK.

class Module {
public:

  void tock() {
    my_sig = my_reg;
    tick();
  }

private:

  void tick() {
    my_reg = 1;
  }

  logic<1> my_sig;
  logic<1> my_reg;
};
