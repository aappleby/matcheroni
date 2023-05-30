#include "metron/tools/metron_tools.h"

// Tocks should be able to call private tasks and functions

class Module {
public:

  logic<8> my_signal;

  int tock() {
    return set_signal(get_number());
  }

  logic<8> get_number() const {
    return 7;
  }

  int set_signal(logic<8> number) {
    my_signal = number;
    return my_signal;
  }

};
