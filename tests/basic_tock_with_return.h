#include "metron/tools/metron_tools.h"

// Tock methods can return values.

class Module {
public:

  int my_sig;

  int tock() {
    my_sig = 7;
    return my_sig;
  }
};
