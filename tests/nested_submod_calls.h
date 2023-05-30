#include "metron/tools/metron_tools.h"

// Nesting submod calls should put all the nested input bindings above the call.

class Submod1 {
public:
  logic<8> add_one(logic<8> a) const {
    return a + 1;
  }
};

class Submod2 {
public:
  logic<8> add_two(logic<8> a) const {
    return a + 1;
  }
};

class Module {
public:

  logic<8> tock(logic<8> old_counter) {
    // Two bindings should end up here.
    logic<8> new_counter = submod1.add_one(submod2.add_two(old_counter));
    return new_counter;
  }

private:

  Submod1 submod1;
  Submod2 submod2;
};
