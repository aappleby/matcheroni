class Metron {
public:

  int my_sig;

  // Divide and mod work, but make Yosys extremely slow to synth

  void tock() {
    int x = 7;
    x += 13;
    x -= 13;
    x *= 13;
    //x /= 13;
    //x %= 13;
    my_sig = x;
  }

  int my_reg1;
  int my_reg2;
  int my_reg3;
  int my_reg4;
  int my_reg5;

  void tick() {
    my_reg1 += 22;
    my_reg2 -= 22;
    my_reg3 *= 22;
    //my_reg4 /= 22;
    //my_reg5 %= 22;
  }
};
