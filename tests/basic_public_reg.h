#include "metron/tools/metron_tools.h"

// Public register member variables get moved to the output port list.

class Module {
public:
  void tock() {
    tick();
  }
  logic<1> my_reg;

private:
  void tick() {
    my_reg = my_reg + 1;
  }
};
