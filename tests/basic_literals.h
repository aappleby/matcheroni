#include "metron/tools/metron_tools.h"

// Number literals
// don't forget the ' spacers

class Module {
public:

  int test1() {
    logic<32> a = 0b0;
    logic<32> b = 0b00;
    logic<32> c = 0b000;
    logic<32> d = 0b0000;
    logic<32> e = 0b00000;
    logic<32> f = 0b000000;
    logic<32> g = 0b0000000;
    logic<32> h = 0b00000000;
    return 0;
  }

  int test2() {
    logic<32> a = 0b0;
    logic<32> b = 0b0'0;
    logic<32> c = 0b0'00;
    logic<32> d = 0b00'00;
    logic<32> e = 0b00'000;
    logic<32> f = 0b0'000'00;
    logic<32> g = 0b000'0000;
    logic<32> h = 0b0'0'0'0'0'0'0'0;
    return 0;
  }

};
