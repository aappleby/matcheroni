#include "metron/tools/metron_tools.h"

class Module {
  public:

  // decl in for
  int loop1() {
    int x = 0;
    for (int i = 0; i < 10; i++) {
      x = x + i;
    }
    return x;
  }

  // decl outside of for
  int loop2() {
    int x = 0;
    int i;
    for (i = 0; i < 10; i++) {
      x = x + i;
    }
    return x;
  }
};
