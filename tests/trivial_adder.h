#include "metron/tools/metron_tools.h"

// Trivial adder just for example.

class Adder {
public:

  logic<8> add(logic<8> a, logic<8> b) {
    return a + b;
  }

};
