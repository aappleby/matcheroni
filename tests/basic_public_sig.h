#include "metron/tools/metron_tools.h"

// Public signal member variables get moved to the output port list.

class Module {
public:

  void tock() {
    my_sig = 1;
  }

  logic<1> my_sig;
};
