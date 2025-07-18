#include "systemc.h"

#include "../cnm_base.h"
#include "../half.hpp"

SC_MODULE(softsimd_pu_driver) {
    sc_out<bool>                rst;
    sc_out<bool>                RD;         // DRAM read command
    sc_out<bool>                WR;         // DRAM write command
    sc_out<bool>                ACT;        // DRAM activate command
//    sc_out<bool>                  RSTB;       //
    sc_out<bool>                AB_mode;    // Signals if the All-Banks mode is enabled
    sc_out<bool>                pim_mode;   // Signals if the PIM mode is enabled
    sc_out<sc_uint<BANK_BITS> > bank_addr;  // Address of the bank
    sc_out<sc_uint<ROW_BITS> >  row_addr;   // Address of the bank row
    sc_out<sc_uint<COL_BITS> >  col_addr;   // Address of the bank column
    sc_out<uint64_t>            DQ;         // Data input from DRAM controller (output makes no sense)
#if DUAL_BANK_INTERFACE
    sc_inout_rv<DRAM_BITS>      even_bus;   // Direct data in/out to the even bank
    sc_inout_rv<DRAM_BITS>      odd_bus;    // Direct data in/out to the odd bank
#else
    sc_inout_rv<DRAM_BITS>      dram_bus;   // Direct data in/out to the DRAM
#endif

    SC_CTOR(softsimd_pu_driver) {
        SC_THREAD(driver_thread);
    }

    void driver_thread();
};
