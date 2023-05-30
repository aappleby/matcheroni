#include "metron/tools/metron_tools.h"

// Writing a register multiple times in the same function is OK.

class Module {
public:

  void tock() {
    my_sig = 0;
    my_sig = 1;
    logic<1> temp = my_sig;
  }

  logic<1> my_sig;
};
