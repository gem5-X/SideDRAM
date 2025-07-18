#include "systemc.h"
#include <bitset>

#include "../cnm_base.h"

SC_MODULE(softsimd_pu_monitor) {

    sc_in_clk                   clk;
    sc_in<bool>                 rst;
    sc_in<bool>                 RD;         // DRAM read command
    sc_in<bool>                 WR;         // DRAM write command
    sc_in<bool>                 ACT;        // DRAM activate command
//    sc_in<bool>                   RSTB;       //
    sc_in<bool>                 AB_mode;    // Signals if the All-Banks mode is enabled
    sc_in<bool>                 pim_mode;   // Signals if the PIM mode is enabled
    sc_in<sc_uint<BANK_BITS> >  bank_addr;  // Address of the bank
    sc_in<sc_uint<ROW_BITS> >   row_addr;   // Address of the bank row
    sc_in<sc_uint<COL_BITS> >   col_addr;   // Address of the bank column
    sc_in<uint64_t>             DQ;         // Data input from DRAM controller (output makes no sense)
#if DUAL_BANK_INTERFACE
    sc_inout_rv<DRAM_BITS>      even_bus;   // Direct data in/out to the even bank
    sc_inout_rv<DRAM_BITS>      odd_bus;    // Direct data in/out to the odd bank
#else
    sc_inout_rv<DRAM_BITS>      dram_bus;   // Direct data in/out to the DRAM
#endif

    // Internal events

    // Internal variables and signals for checking

    SC_CTOR(softsimd_pu_monitor) {
        SC_THREAD(monitor_thread);
        sensitive << clk.pos() << rst.neg();
    }

    void monitor_thread(); // Outputs the behavior and automatically checks the functionality
};
