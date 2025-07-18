#include "systemc.h"

#include "../cnm_base.h"

class pch_driver: public sc_module {
public:

#if MIXED_SIM

    sc_out<sc_logic>            rst;
    sc_out<sc_logic>            RD;						// DRAM read command
    sc_out<sc_logic>            WR;						// DRAM write command
    sc_out<sc_logic>            ACT;					// DRAM activate command
//    sc_out<sc_logic>				RSTB;					//
    sc_out<sc_logic>            AB_mode;			    // Signals if the All-Banks mode is enabled
    sc_out<sc_logic>            pim_mode;				// Signals if the PIM mode is enabled
    sc_out<sc_lv<BANK_BITS> > 	bank_addr;				// Address of the bank
    sc_out<sc_lv<ROW_BITS> >  	row_addr;			    // Address of the bank row
    sc_out<sc_lv<COL_BITS> >  	col_addr;		        // Address of the bank column
    sc_out<sc_lv<64> >          DQ;                     // Data input from DRAM controller (output makes no sense)
#if DUAL_BANK_INTERFACE
    sc_out<sc_lv<DRAM_BITS> >	even_in[CORES_PER_PCH];	// Direct data in/out to the even banks
    sc_out<sc_lv<DRAM_BITS> >   odd_in[CORES_PER_PCH];  // Direct data in/out to the odd banks
    sc_in<sc_lv<DRAM_BITS> >	even_out[CORES_PER_PCH];// Direct data in/out to the even banks
    sc_in<sc_lv<DRAM_BITS> >    odd_out[CORES_PER_PCH]; // Direct data in/out to the odd banks
#else
    sc_out<sc_lv<DRAM_BITS> >   dram_in[CORES_PER_PCH]; // Direct data in/out to the even banks
    sc_in<sc_lv<DRAM_BITS> >    dram_out[CORES_PER_PCH];// Direct data in/out to the odd banks
#endif

#else   // MIXED_SIM

    sc_out<bool>                rst;
    sc_out<bool>                RD;							// DRAM read command
    sc_out<bool>                WR;							// DRAM write command
    sc_out<bool>                ACT;						// DRAM activate command
//    sc_out<bool>				RSTB;						//
    sc_out<bool>                AB_mode;			        // Signals if the All-Banks mode is enabled
    sc_out<bool>                pim_mode;				    // Signals if the PIM mode is enabled
    sc_out<sc_uint<BANK_BITS> > bank_addr;				    // Address of the bank
    sc_out<sc_uint<ROW_BITS> >  row_addr;			        // Address of the bank row
    sc_out<sc_uint<COL_BITS> >  col_addr;		            // Address of the bank column
    sc_out<uint64_t>            DQ;                         // Data input from DRAM controller (output makes no sense)
#if DUAL_BANK_INTERFACE
    sc_inout_rv<DRAM_BITS>      even_buses[CORES_PER_PCH];  // Direct data in/out to the even banks
    sc_inout_rv<DRAM_BITS>      odd_buses[CORES_PER_PCH];   // Direct data in/out to the odd banks
#else
    sc_inout_rv<DRAM_BITS>      dram_buses[CORES_PER_PCH];  // Direct data in/out to the banks
#endif

#endif  // MIXED_SIM

    std::string filename;
    SC_HAS_PROCESS(pch_driver);
    pch_driver(sc_module_name name_, std::string filename_) : sc_module(name_), filename(filename_) {
        SC_THREAD(driver_thread);
    }

    void driver_thread();
};
