#include "metron/tools/metron_tools.h"

// Zero-initializing structs should work for convenience.

struct MyStruct1 {
  logic<8> field;
};

class Module {
public:

  MyStruct1 my_struct1;
  void tock() {
    // FIXME fix this later glarghbh
    //my_struct1 = {0};
    //my_struct1.field = 1;
  }
};
