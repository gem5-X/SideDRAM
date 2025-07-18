/*
 * Copyright EPFL 2021
 * Rafael Medina Morillas
 *
 * Description of a pseudo-Channel that contains several IMC cores
 *
 */

#ifndef IMC_PCH_H_
#define IMC_PCH_H_

#include "cnm_base.h"
#include "softsimd_pu.h"

class imc_pch: public sc_module {
public:

#ifdef __SYNTHESIS__

    sc_in_clk					clk;
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
    sc_in<sc_lv<DRAM_BITS> >    even_in[CORES_PER_PCH];		// Direct data in/out to the even bank
	sc_in<sc_lv<DRAM_BITS> >	odd_in[CORES_PER_PCH];		// Direct data in/out to the odd bank
	sc_out<sc_lv<DRAM_BITS> >	even_out[CORES_PER_PCH];	// Direct data in/out to the even bank
	sc_out<sc_lv<DRAM_BITS> >	odd_out[CORES_PER_PCH];		// Direct data in/out to the odd bank
#else
    sc_in<sc_lv<DRAM_BITS> >    dram_in[CORES_PER_PCH];     // Direct data in/out to the bank
    sc_out<sc_lv<DRAM_BITS> >   dram_out[CORES_PER_PCH];    // Direct data in/out to the bank
#endif


    // ** INTERNAL SIGNALS AND VARIABLES **

    // Auxiliar signals

    // Internal modules

    SC_CTOR(imc_pch) {

        uint i;

        softsimd_pu *imc_cores[CORES_PER_PCH];	// Vector of IMC cores

        for (i = 0; i < CORES_PER_PCH; i++) {
            imc_cores[i] = new softsimd_pu(sc_gen_unique_name("imc_core"));
            imc_cores[i]->clk(clk);
            imc_cores[i]->rst(rst);
            imc_cores[i]->RD(RD);
            imc_cores[i]->WR(WR);
            imc_cores[i]->ACT(ACT);
//			imc_cores[i]->RSTB(RSTB);
            imc_cores[i]->AB_mode(AB_mode);
            imc_cores[i]->pim_mode(pim_mode);
            imc_cores[i]->bank_addr(bank_addr);
            imc_cores[i]->row_addr(row_addr);
            imc_cores[i]->col_addr(col_addr);
            imc_cores[i]->DQ(DQ);
#if DUAL_BANK_INTERFACE
            imc_cores[i]->even_in(even_in[i]);
            imc_cores[i]->odd_in(odd_in[i]);
            imc_cores[i]->even_out(even_out[i]);
            imc_cores[i]->odd_out(odd_out[i]);
#else
            imc_cores[i]->dram_in(dram_in[i]);
            imc_cores[i]->dram_out(dram_out[i]);
#endif
        }

    }

#else

    sc_in_clk					clk;
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
    sc_inout_rv<DRAM_BITS>      dram_buses[CORES_PER_PCH];  // Direct data in/out to the DRAM
#endif

    // ** INTERNAL SIGNALS AND VARIABLES **

    // Auxiliar signals

    // Internal modules
    softsimd_pu *imc_cores[CORES_PER_PCH];   // Vector of IMC cores

    SC_HAS_PROCESS(imc_pch);
#if (RECORDING || EN_MODEL)
    std::string filename;
    imc_pch(sc_module_name name, std::string filename_) : sc_module(name), filename(filename_) {
#else
    imc_pch(sc_module_name name) : sc_module(name) {
#endif

        uint i;

        for (i = 0; i < CORES_PER_PCH; i++) {
#if (RECORDING || EN_MODEL)
            imc_cores[i] = new softsimd_pu(sc_gen_unique_name("softsimd_pu"), filename+"_"+std::to_string(i));
#else
            imc_cores[i] = new softsimd_pu(sc_gen_unique_name("softsimd_pu"));
#endif
            imc_cores[i]->clk(clk);
            imc_cores[i]->rst(rst);
            imc_cores[i]->RD(RD);
            imc_cores[i]->WR(WR);
            imc_cores[i]->ACT(ACT);
//			imc_cores[i]->RSTB(RSTB);
            imc_cores[i]->AB_mode(AB_mode);
            imc_cores[i]->pim_mode(pim_mode);
            imc_cores[i]->bank_addr(bank_addr);
            imc_cores[i]->row_addr(row_addr);
            imc_cores[i]->col_addr(col_addr);
            imc_cores[i]->DQ(DQ);
#if (DUAL_BANK_INTERFACE)
            imc_cores[i]->even_bus(even_buses[i]);
            imc_cores[i]->odd_bus(odd_buses[i]);
#else
            imc_cores[i]->dram_bus(dram_buses[i]);
#endif
        }

    }

#endif

};

#endif /* IMC_PCH_H_ */
