#include "metron/tools/metron_tools.h"

// Private getter methods are OK

class Module {
public:

  int my_sig;

  int tock() {
    my_sig = my_getter();
    return my_sig;
  }

private:

  int my_getter() const {
    return 12;
  }

};
