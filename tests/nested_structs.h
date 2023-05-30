#include "metron/tools/metron_tools.h"

struct InnerStruct {
  logic<8> a;
  logic<8> b;
  logic<8> c;
};

struct OuterStruct {
  InnerStruct x;
  InnerStruct y;
  InnerStruct z;
};

class Module {
public:

  OuterStruct s;

  void func1() {
    s.x.a = 1;
    s.x.b = 2;
    s.x.c = 3;
  }

  void func2() {
    s.y.a = 4;
    s.y.b = 5;
    s.y.c = 6;
  }

  void func3() {
    s.z.a = 7;
    s.z.b = 8;
    s.z.c = 9;
  }
};
