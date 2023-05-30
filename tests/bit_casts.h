#include "metron/tools/metron_tools.h"

// Logics can be casted to various sizes via bN() or bx<N>()

class Module {
public:

  int test_bN() {
    logic<64> src = 0x1234567812345678;
    logic<64> dst64 = b64(src);
    logic<63> dst63 = b63(src);
    logic<62> dst62 = b62(src);
    logic<61> dst61 = b61(src);
    logic<60> dst60 = b60(src);

    logic<59> dst59 = b59(src);
    logic<58> dst58 = b58(src);
    logic<57> dst57 = b57(src);
    logic<56> dst56 = b56(src);
    logic<55> dst55 = b55(src);
    logic<54> dst54 = b54(src);
    logic<53> dst53 = b53(src);
    logic<52> dst52 = b52(src);
    logic<51> dst51 = b51(src);
    logic<50> dst50 = b50(src);

    logic<49> dst49 = b49(src);
    logic<48> dst48 = b48(src);
    logic<47> dst47 = b47(src);
    logic<46> dst46 = b46(src);
    logic<45> dst45 = b45(src);
    logic<44> dst44 = b44(src);
    logic<43> dst43 = b43(src);
    logic<42> dst42 = b42(src);
    logic<41> dst41 = b41(src);
    logic<40> dst40 = b40(src);

    logic<39> dst39 = b39(src);
    logic<38> dst38 = b38(src);
    logic<37> dst37 = b37(src);
    logic<36> dst36 = b36(src);
    logic<35> dst35 = b35(src);
    logic<34> dst34 = b34(src);
    logic<33> dst33 = b33(src);
    logic<32> dst32 = b32(src);
    logic<31> dst31 = b31(src);
    logic<30> dst30 = b30(src);

    logic<29> dst29 = b29(src);
    logic<28> dst28 = b28(src);
    logic<27> dst27 = b27(src);
    logic<26> dst26 = b26(src);
    logic<25> dst25 = b25(src);
    logic<24> dst24 = b24(src);
    logic<23> dst23 = b23(src);
    logic<22> dst22 = b22(src);
    logic<21> dst21 = b21(src);
    logic<20> dst20 = b20(src);

    logic<19> dst19 = b19(src);
    logic<18> dst18 = b18(src);
    logic<17> dst17 = b17(src);
    logic<16> dst16 = b16(src);
    logic<15> dst15 = b15(src);
    logic<14> dst14 = b14(src);
    logic<13> dst13 = b13(src);
    logic<12> dst12 = b12(src);
    logic<11> dst11 = b11(src);
    logic<10> dst10 = b10(src);

    logic<9> dst9 = b9(src);
    logic<8> dst8 = b8(src);
    logic<7> dst7 = b7(src);
    logic<6> dst6 = b6(src);
    logic<5> dst5 = b5(src);
    logic<4> dst4 = b4(src);
    logic<3> dst3 = b3(src);
    logic<2> dst2 = b2(src);
    logic<1> dst1 = b1(src);
    return 0;
  }

  int test_bx_const() {
    logic<64> src = 0x1234567812345678;
    logic<63> dst63 = bx<63>(src);
    logic<62> dst62 = bx<62>(src);
    logic<61> dst61 = bx<61>(src);
    logic<60> dst60 = bx<60>(src);

    logic<59> dst59 = bx<59>(src);
    logic<58> dst58 = bx<58>(src);
    logic<57> dst57 = bx<57>(src);
    logic<56> dst56 = bx<56>(src);
    logic<55> dst55 = bx<55>(src);
    logic<54> dst54 = bx<54>(src);
    logic<53> dst53 = bx<53>(src);
    logic<52> dst52 = bx<52>(src);
    logic<51> dst51 = bx<51>(src);
    logic<50> dst50 = bx<50>(src);

    logic<49> dst49 = bx<49>(src);
    logic<48> dst48 = bx<48>(src);
    logic<47> dst47 = bx<47>(src);
    logic<46> dst46 = bx<46>(src);
    logic<45> dst45 = bx<45>(src);
    logic<44> dst44 = bx<44>(src);
    logic<43> dst43 = bx<43>(src);
    logic<42> dst42 = bx<42>(src);
    logic<41> dst41 = bx<41>(src);
    logic<40> dst40 = bx<40>(src);

    logic<39> dst39 = bx<39>(src);
    logic<38> dst38 = bx<38>(src);
    logic<37> dst37 = bx<37>(src);
    logic<36> dst36 = bx<36>(src);
    logic<35> dst35 = bx<35>(src);
    logic<34> dst34 = bx<34>(src);
    logic<33> dst33 = bx<33>(src);
    logic<32> dst32 = bx<32>(src);
    logic<31> dst31 = bx<31>(src);
    logic<30> dst30 = bx<30>(src);

    logic<29> dst29 = bx<29>(src);
    logic<28> dst28 = bx<28>(src);
    logic<27> dst27 = bx<27>(src);
    logic<26> dst26 = bx<26>(src);
    logic<25> dst25 = bx<25>(src);
    logic<24> dst24 = bx<24>(src);
    logic<23> dst23 = bx<23>(src);
    logic<22> dst22 = bx<22>(src);
    logic<21> dst21 = bx<21>(src);
    logic<20> dst20 = bx<20>(src);

    logic<19> dst19 = bx<19>(src);
    logic<18> dst18 = bx<18>(src);
    logic<17> dst17 = bx<17>(src);
    logic<16> dst16 = bx<16>(src);
    logic<15> dst15 = bx<15>(src);
    logic<14> dst14 = bx<14>(src);
    logic<13> dst13 = bx<13>(src);
    logic<12> dst12 = bx<12>(src);
    logic<11> dst11 = bx<11>(src);
    logic<10> dst10 = bx<10>(src);

    logic<9> dst9 = bx<9>(src);
    logic<8> dst8 = bx<8>(src);
    logic<7> dst7 = bx<7>(src);
    logic<6> dst6 = bx<6>(src);
    logic<5> dst5 = bx<5>(src);
    logic<4> dst4 = bx<4>(src);
    logic<3> dst3 = bx<3>(src);
    logic<2> dst2 = bx<2>(src);
    logic<1> dst1 = bx<1>(src);
    return 0;
  }

  int test_bN_offset() {
    logic<64> src = 0x1234567812345678;

    logic<8> dst0 = b8(src, 0);
    logic<8> dst1 = b8(src, 1);
    logic<8> dst2 = b8(src, 2);
    logic<8> dst3 = b8(src, 3);
    logic<8> dst4 = b8(src, 4);
    logic<8> dst5 = b8(src, 5);
    logic<8> dst6 = b8(src, 6);
    logic<8> dst7 = b8(src, 7);
    logic<8> dst8 = b8(src, 8);
    logic<8> dst9 = b8(src, 9);
    return 0;
  }

  static const int some_size1 = 64;
  static const int some_size2 = 8;

  int test_bx_param() {
    logic<some_size1> a = 10;
    logic<some_size2> b = bx<some_size2>(a);

    logic<some_size2> b0 = bx<some_size2>(a, 0);
    logic<some_size2> b1 = bx<some_size2>(a, 1);
    logic<some_size2> b2 = bx<some_size2>(a, 2);
    logic<some_size2> b3 = bx<some_size2>(a, 3);
    logic<some_size2> b4 = bx<some_size2>(a, 4);
    logic<some_size2> b5 = bx<some_size2>(a, 5);
    logic<some_size2> b6 = bx<some_size2>(a, 6);
    logic<some_size2> b7 = bx<some_size2>(a, 7);
    logic<some_size2> b8 = bx<some_size2>(a, 8);
    logic<some_size2> b9 = bx<some_size2>(a, 9);

    return 0;
  }


  int test2() {
    logic<32> a = 0xDEADBEEF;

    logic<1> b = b1(a, 3); //static bit extract with literal offset, width 1
    logic<7> c = b7(a, 3); //static bit extract with literal offset, width N

    logic<1> e = b1(a, some_size1); //static bit extract with variable offset, width 1
    logic<7> f = b7(a, some_size2); //static bit extract with variable offset, width N

    return 0;
  }



};
