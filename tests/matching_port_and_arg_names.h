#include "metron/tools/metron_tools.h"

// Port and function arg names can collide, the latter is disambiguated by its
// function name.

class Module {
public:

  int input_val;
  int output1;
  int output2;

  void tock1() {
    output1 = input_val + 7;
  }

  void tock2(int input_val) {
    output2 = input_val + 8;
  }
};
