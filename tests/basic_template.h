#include "metron/tools/metron_tools.h"

// Templates can be used for module parameters

template<int WIDTH = 123, int HEIGHT = 456>
class Submod {
public:

  void tock(logic<WIDTH> dx, logic<HEIGHT> dy) {
    // my parser is seeing "bx<WIDTH>(100)"
    // as "bx LT (width GT (100))"...
    my_width = bx<WIDTH>(100) + dx;
    my_height = bx<HEIGHT>(200) + dy;
  }

  logic<WIDTH> my_width;
  logic<HEIGHT> my_height;
};

class Module {
public:

  logic<20> tock() {
    submodule.tock(1, 2);
    return submodule.my_width + submodule.my_height;
  }

  Submod<10,11> submodule;
};
