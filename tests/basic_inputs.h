#include "metron/tools/metron_tools.h"

// Tick inputs should become input ports.

// clang-format off
class Module {
 public:

  void tock(logic<7> my_input) {
    tick(my_input);
  }

 private:

  void tick(logic<7> my_input) {
    my_reg = my_reg + my_input;
  }

  logic<7> my_reg;
};
// clang-format on
