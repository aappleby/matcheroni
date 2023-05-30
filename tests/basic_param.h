#include "metron/tools/metron_tools.h"

// Template parameters become SV module parameters.

template<int SOME_CONSTANT = 7>
class Module {
public:

  void tock() {
    tick();
  }

private:

  void tick() {
    my_reg = my_reg + SOME_CONSTANT;
  }

  logic<7> my_reg;
};
