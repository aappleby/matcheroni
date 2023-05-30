#include "metron/tools/metron_tools.h"

// Simple switch statements are OK.

class Module {
public:

  void tock(logic<2> selector) {
    tick(selector);
  }

private:

  void tick(logic<2> selector) {
    switch(selector) {
      case 0: // comment
        my_reg = 17; break;
      case 1:  // comment
        my_reg = 22; break;
      case 2: my_reg = 30; break;
      case 3: // fallthrough
      case 4:
      case 5:
      case 6: my_reg = 72; break;
    }
  }

  logic<8> my_reg;
};
