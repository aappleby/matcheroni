#include "metron/tools/metron_tools.h"

class Module {
public:

  int test_dup1() {
    logic<1> a1 = 0b1;
    logic<1> b1 = dup<1>(a1);
    return 0;
  }

  int test_dup4() {
    logic<1>  a1 = 0b1;
    logic<4>  b1 = dup<4>(a1);

    logic<2>  a2 = 0b01;
    logic<8>  b2 = dup<4>(a2);

    logic<3>  a3 = 0b001;
    logic<12> b3 = dup<4>(a3);

    logic<4>  a4 = 0b0001;
    logic<16> b4 = dup<4>(a4);

    logic<5>  a5 = 0b00001;
    logic<20> b5 = dup<4>(a5);

    logic<6>  a6 = 0b000001;
    logic<24> b6 = dup<4>(a6);

    logic<7>  a7 = 0b0000001;
    logic<28> b7 = dup<4>(a7);

    logic<8>  a8 = 0b00000001;
    logic<32> b8 = dup<4>(a8);
    return 0;
  }
};
