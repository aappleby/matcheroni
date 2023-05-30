#include "metron/tools/metron_tools.h"

// Basic #defines should work as long as their contents are simultaneously
// valid C++ and SV

#define MY_CONSTANT1 10
#define MY_CONSTANT2 20
#define MY_OTHER_CONSTANT (MY_CONSTANT1 + MY_CONSTANT2 + 7)

class Module {
public:

  logic<8> test() {
    return MY_OTHER_CONSTANT;
  }

};
