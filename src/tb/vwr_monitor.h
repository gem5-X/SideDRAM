#include "systemc"

#include "../cnm_base.h"

SC_MODULE(vwr_monitor) {
    sc_in<bool>                 clk;
    sc_in<bool>                 rst;
    sc_in<bool>                 wr_nrd;
    sc_in<uint>                 idx;
    sc_in<bool>                 d_nm;
//    sc_in<uint>                 pos;
//    sc_in<uint>                 len;
    sc_in<sc_lv<VWR_BITS> >    lbus;
    sc_in<sc_lv<WORD_BITS> >    sbus;

    SC_CTOR(vwr_monitor) {
        SC_THREAD(monitor_thread);
        sensitive << clk.pos() << rst.neg();
    }

    void monitor_thread();
};
