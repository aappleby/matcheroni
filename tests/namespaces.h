#include "metron/tools/metron_tools.h"

// Namespaces turn into packages.
// "using" doesn't work in methods right now :/

namespace MyPackage {
  static const int foo = 3;
};

class Module {
public:

  int my_sig;
  int my_reg;

  int tock() {
    my_sig = MyPackage::foo + 1;
    return my_sig;
  }

  void tick() {
    my_reg = my_reg + MyPackage::foo;
  }
};
