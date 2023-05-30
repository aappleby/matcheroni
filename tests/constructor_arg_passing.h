#include "metron/tools/metron_tools.h"

// We should be able to pass constructor arguments to submodules.

//----------------------------------------

template<int data_len = 1024, int blarp = 0>
class Module {
public:

  Module(const char* filename = nullptr, logic<10> default_addr = 0x0000) {
    if (filename) readmemh(filename, data);
    addr = default_addr;
  }

  void tock(logic<10> new_addr) {
    addr = new_addr;
  }

  void tick() {
    out = data[addr];
  }

  logic<8> get_data() {
    return out;
  }

private:
  logic<10> addr;
  logic<8> data[data_len];
  logic<8> out;
};

//----------------------------------------

class Top {
public:
  Top() : mod("examples/uart/message.hex", 0x1234), derp(7) {
  }

  void tock(logic<10> addr) {
    mod.tock(addr);
  }

  void tick() {
    mod.tick();
  }

  Module<7777, 8383> mod;
  logic<10> derp;
};

//----------------------------------------
