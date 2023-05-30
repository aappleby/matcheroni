#include "metron/tools/metron_tools.h"

// Structs exist in both C++ and Verilog

struct MyStruct1 {
  logic<8> a;
};

class Module {
public:

  MyStruct1 my_struct1;

  logic<8> tock() {
    my_struct1.a = 1;
    return 17;
  }
};
