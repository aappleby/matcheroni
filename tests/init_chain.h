#include "metron/tools/metron_tools.h"

// I don't know why you would want to do this, but it should work.

class Module {
public:
  Module() {
    init1();
  }

  int tock() {
    tick();
    return 0;
  }

private:

  void tick() {
    reg1 = reg1 + 1;
    reg2 = reg2 + 1;
    reg3 = reg3 + 1;
    reg4 = reg4 + 1;
    reg5 = reg5 + 1;
  }

  logic<8> reg1;
  logic<8> reg2;
  logic<8> reg3;
  logic<8> reg4;
  logic<8> reg5;

  void init1() {
    reg1 = 1;
    init2();
  }

  void init2() {
    reg2 = 2;
    init3();
  }

  void init3() {
    reg3 = 3;
    init4();
  }

  void init4() {
    reg4 = 4;
    init5();
  }

  void init5() {
    reg5 = 5;
  }

};
