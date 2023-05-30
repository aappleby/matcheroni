#include "metron/tools/metron_tools.h"

// Comments surrounded by / * # <something # * / get unwrapped and dropped
// directly in the output file.

class Module {
public:

  void tick() {
    my_reg = my_reg + 1;
  }

/*#
  always @(posedge clock) begin
    //$display("Hello World!\n");
  end
#*/

  int my_reg;
};
