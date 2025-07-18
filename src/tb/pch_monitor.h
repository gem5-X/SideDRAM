#include "systemc.h"
#include <bitset>

#include "../cnm_base.h"

SC_MODULE(pch_monitor) {

#if MIXED_SIM

	sc_in<sc_logic>             clk;
    sc_in<sc_logic>             rst;
    sc_in<sc_logic>             RD;						// DRAM read command
    sc_in<sc_logic>             WR;						// DRAM write command
    sc_in<sc_logic>             ACT;					// DRAM activate command
//    sc_in<sc_logic>			    RSTB;				    //
    sc_in<sc_logic>             AB_mode;			    // Signals if the All-Banks mode is enabled
    sc_in<sc_logic>             pim_mode;				// Signals if the PIM mode is enabled
    sc_in<sc_lv<BANK_BITS> >	bank_addr;				// Address of the bank
    sc_in<sc_lv<ROW_BITS> >     row_addr;				// Address of the bank row
    sc_in<sc_lv<COL_BITS> >     col_addr;			    // Address of the bank column
    sc_in<sc_lv<64> >           DQ;	                    // Data input from DRAM controller (output makes no sense)
#if DUAL_BANK_INTERFACE
    sc_in<sc_lv<DRAM_BITS> >	even_in[CORES_PER_PCH];	// Direct data in/out to the even banks
    sc_in<sc_lv<DRAM_BITS> >    odd_in[CORES_PER_PCH];  // Direct data in/out to the odd banks
    sc_in<sc_lv<DRAM_BITS> >	even_out[CORES_PER_PCH];// Direct data in/out to the even banks
    sc_in<sc_lv<DRAM_BITS> >    odd_out[CORES_PER_PCH]; // Direct data in/out to the odd banks
#else
    sc_in<sc_lv<DRAM_BITS> >    dram_in[CORES_PER_PCH]; // Direct data in/out to the even banks
    sc_in<sc_lv<DRAM_BITS> >    dram_out[CORES_PER_PCH];// Direct data in/out to the odd banks
#endif

#else   // MIXED_SIM

    sc_in_clk                   clk;
    sc_in<bool>                 rst;
    sc_in<bool>                 RD;							// DRAM read command
    sc_in<bool>                 WR;							// DRAM write command
    sc_in<bool>                 ACT;						// DRAM activate command
//    sc_in<bool>					RSTB;						//
    sc_in<bool>                 AB_mode;			        // Signals if the All-Banks mode is enabled
    sc_in<bool>                 pim_mode;				    // Signals if the PIM mode is enabled
    sc_in<sc_uint<BANK_BITS> >  bank_addr;				    // Address of the bank
    sc_in<sc_uint<ROW_BITS> >   row_addr;				    // Address of the bank row
    sc_in<sc_uint<COL_BITS> >   col_addr;			        // Address of the bank column
    sc_in<uint64_t>             DQ;	                        // Data input from DRAM controller (output makes no sense)
#if DUAL_BANK_INTERFACE
    sc_inout_rv<DRAM_BITS>      even_buses[CORES_PER_PCH];  // Direct data in/out to the even banks
    sc_inout_rv<DRAM_BITS>      odd_buses[CORES_PER_PCH];   // Direct data in/out to the odd banks
#else
    sc_inout_rv<DRAM_BITS>      dram_buses[CORES_PER_PCH];  // Direct data in/out to the banks
#endif

#endif  // MIXED_SIM

    // Internal events

    // Internal variables and signals for checking

    SC_CTOR(pch_monitor) {
        SC_THREAD(monitor_thread);
        sensitive << clk.pos() << rst.neg();
    }

    void monitor_thread(); // Outputs the behavior and automatically checks the functionality
//    void mirror_thread();		// Mirror IMC core behavior for checking its functionality
//    void out_mirror_thread();	// Mirror IMC core output for checking functionality
};
