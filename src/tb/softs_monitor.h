#include "systemc"

#include "../cnm_base.h"

SC_MODULE(softs_monitor) {
    sc_in<bool>         clk;
    sc_in<bool>         rst;
    // Shift & Add
    sc_in<bool>         SA_en;
    sc_in<uint>         SA_shift;
    sc_in<uint>         SA_size;
    sc_in<bool>         SA_s_na;
    sc_in<uint64_t>     SA_op1[WORD_64B];
    sc_in<uint64_t>     SA_op2[WORD_64B];
    sc_in<uint64_t>     SA_out[WORD_64B];
    // Pack & Mask
    sc_in<bool>         PM_en;
    sc_in<uint64_t>     PM_w1[WORD_64B];
    sc_in<uint64_t>     PM_w2[WORD_64B];
    sc_in<uint>         PM_in_size;
    sc_in<uint>         PM_out_size;
    sc_in<uint>         PM_in_start;
    sc_in<uint64_t>     PM_mask_in[MASK_64B];
    sc_in<uint8_t>      PM_op_sel;
    sc_in<uint>         PM_shift;
    sc_in<uint64_t>     PM_out[WORD_64B];

    SC_CTOR(softs_monitor) {
        SC_THREAD(monitor_thread);
        sensitive << clk.pos() << rst.neg();
    }

    void monitor_thread();
};
