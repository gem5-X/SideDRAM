#include "systemc"

#include "../cnm_base.h"

SC_MODULE(ms_monitor) {
    sc_in<bool>         clk;
    sc_in<bool>         rst;
    sc_in<bool>         enable;             // Signals if macroinstruction buffer is pointing here
    sc_in<OPC_STORAGE>  dst;                // DST specified by macroinstruction
    sc_in<uint64_t>     csd_in[CSD_64B];    // CSD multiplier input
    sc_in<uint8_t>      csd_len;            // Length of CSD multiplier input
    sc_in<bool>         sa_valid;           // Signals if S&A index is valid
    sc_in<uint>         sa_index;           // Index for the S&A DLB microcode
    sc_in<OPC_STORAGE>  sa_src0;            // Sets SRC0 for the current microinstruction
    sc_in<OPC_STORAGE>  sa_dst;             // Sets DST for the current microinstruction
    sc_out<uint>        dst_uint;
    sc_out<uint>        sa_src0_uint;
    sc_out<uint>        sa_dst_uint;

    SC_CTOR(ms_monitor) {
        SC_THREAD(monitor_thread);
        sensitive << clk.pos() << rst.neg();

        SC_METHOD(parser_method);
        sensitive << dst << sa_src0 << sa_dst;
    }

    void monitor_thread();
    void parser_method();
};
