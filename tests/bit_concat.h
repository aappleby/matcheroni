#include "metron/tools/metron_tools.h"

// Concatenating logics should produce logics with correct <N>

class Module {
public:

  Module() {
    write("Hello World?\n");
  }

  logic<6> sig1;
  logic<6> sig2;

  void tock1() {
    logic<1> a = 1;
    logic<2> b = 2;
    logic<3> c = 3;

    sig1 = cat(a, b, c);
  }

  void tock2() {
    logic<8> a = 1;
    logic<8> b = 2;
    logic<8> c = 3;

    sig2 = cat(b1(a), b2(b), b3(c));
  }
};
