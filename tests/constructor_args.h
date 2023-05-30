#include "metron/tools/metron_tools.h"

// We should be able to handle constructor args.

template<int data_len = 1024, int blarp = 7>
class Module {
public:

  Module(const char* filename = "examples/uart/message.hex", logic<10> start_addr = 0) {
    addr = start_addr;
    readmemh(filename, data);
  }

  void tock(logic<10> addr_) {
    addr = addr_;
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
