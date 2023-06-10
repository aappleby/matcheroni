#include "metron/tools/metron_tools.h"

// Logics can be casted to various sizes via bN() or bx<N>()

class Module {
public:
  int test_bx_param() {
    logic<some_size2> b0 = bx<some_size2>(a, 0);
  }
};
