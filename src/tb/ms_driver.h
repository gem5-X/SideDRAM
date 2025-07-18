#include "systemc.h"

#include "../cnm_base.h"

SC_MODULE(ms_driver) {

    sc_out<bool>        rst;
    sc_out<bool>        enable;             // Signals if macroinstruction buffer is pointing here
    sc_out<OPC_STORAGE> dst;                // DST specified by macroinstruction
    sc_out<uint64_t>    csd_in[CSD_64B];    // CSD multiplier input
    sc_out<uint8_t>     csd_len;            // Length of CSD multiplier input

    SC_CTOR(ms_driver) {
        SC_THREAD(driver_thread);
    }

    void driver_thread();
};
