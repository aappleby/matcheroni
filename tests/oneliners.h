#include "metron/tools/metron_tools.h"

// Cramming various statements into one line should not break anything.

class Module {
public:

  logic<8> test() { logic<8> a = 1; a = a + 7; return a; }

  void tick() { if (my_reg & 1) my_reg = my_reg - 7; }

  logic<8> my_reg;

};
