#include "systemc.h"

#include "../cnm_base.h"

SC_MODULE(softs_driver) {

    sc_out<bool>            rst;
    // Shift & Add
    sc_out<bool>            SA_en;
    sc_out<uint>            SA_shift;
    sc_out<uint>            SA_size;
    sc_out<bool>            SA_s_na;
    sc_out<uint64_t>        SA_op1[WORD_64B];
    sc_out<uint64_t>        SA_op2[WORD_64B];
    // Pack & Mask
    sc_out<bool>            PM_en;
    sc_out<uint64_t>        PM_w1[WORD_64B];
    sc_out<uint64_t>        PM_w2[WORD_64B];
    sc_out<uint>            PM_in_size;
    sc_out<uint>            PM_out_size;
    sc_out<uint>            PM_in_start;
    sc_out<uint64_t>        PM_mask_in[MASK_64B];
    sc_out<uint8_t>         PM_op_sel;
    sc_out<uint>            PM_shift;

    SC_CTOR(softs_driver) {
        SC_THREAD(driver_thread);
    }

    void driver_thread();
};

