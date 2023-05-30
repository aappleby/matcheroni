#include "metron/tools/metron_tools.h"

// Modules can use constructors to initialize state

class Module {
public:

  Module() {
    my_reg = 7;
  }

  logic<8> get_reg() {
    return my_reg;
  }

private:

  logic<8> my_reg;
};

/*#
`ifdef IVERILOG
module Test;
  logic clock;
  logic[7:0] tock;
  Module mod(.clock(clock), .tock(tock));
  initial begin
    if (tock != 7) $display("FAIL");
  end
endmodule
`endif
#*/
