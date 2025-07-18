#include "systemc.h"

#include "../cnm_base.h"

SC_MODULE(vwr_driver) {

    sc_out<bool>                rst;
    sc_out<bool>                wr_nrd;
    sc_out<uint>                idx;
    sc_out<bool>                d_nm;
//    sc_out<uint>                pos;
//    sc_out<uint>                len;
    sc_inout_rv<VWR_BITS>      lbus;
    sc_inout_rv<WORD_BITS>      sbus;

    SC_CTOR(vwr_driver) {
        SC_THREAD(driver_thread);
    }

    void driver_thread();
};
