#ifndef PCH_MAIN_H_
#define PCH_MAIN_H_

#include "pch_driver.h"
#include "pch_monitor.h"
#include <string>

#if MIXED_SIM
#include "../softsimd_wrapped.h"
#else
#include "../imc_pch.h"
#endif

#ifdef MTI_SYSTEMC

SC_MODULE(top) {

#if MIXED_SIM

	sc_clock						clk_g;
	sc_signal<sc_logic>				clk;
    sc_signal<sc_logic>             rst;
    sc_signal<sc_logic>             RD;							// DRAM read command
    sc_signal<sc_logic>             WR;							// DRAM write command
    sc_signal<sc_logic>             ACT;						// DRAM activate command
//    sc_signal<sc_logic>			    RSTB;						//
    sc_signal<sc_logic>             AB_mode;		            // Signals if the All-Banks mode is enabled
    sc_signal<sc_logic>             pim_mode;			        // Signals if the PIM mode is enabled
    sc_signal<sc_lv<BANK_BITS> >  	bank_addr;			        // Address of the bank
    sc_signal<sc_lv<ROW_BITS> >   	row_addr;			        // Address of the bank row
    sc_signal<sc_lv<COL_BITS> >   	col_addr;		            // Address of the bank column
    sc_signal<sc_lv<64> >           DQ;	                        // Data input from DRAM controller (output makes no sense
#if DUAL_BANK_INTERFACE
	sc_signal<sc_lv<DRAM_BITS> >    even_in[CORES_PER_PCH];  	// Direct data in/out to the even bank
    sc_signal<sc_lv<DRAM_BITS> >    odd_in[CORES_PER_PCH];   	// Direct data in/out to the odd bank
    sc_signal<sc_lv<DRAM_BITS> >    even_out[CORES_PER_PCH];  	// Direct data in/out to the even bank
    sc_signal<sc_lv<DRAM_BITS> >    odd_out[CORES_PER_PCH];   	// Direct data in/out to the odd bank
#else
    sc_signal<sc_lv<DRAM_BITS> >    dram_in[CORES_PER_PCH];  	// Direct data in/out to the even bank
    sc_signal<sc_lv<DRAM_BITS> >    dram_out[CORES_PER_PCH];   	// Direct data in/out to the odd bank
#endif

	softsimd_pu *dut[CORES_PER_PCH];
	pch_driver driver;
	pch_monitor monitor;

	SC_CTOR(top) : clk_g("clk", CLK_PERIOD, RESOLUTION),
					driver("Driver" ,"debug"), monitor("Monitor")
	{
		uint i;

		SC_METHOD(clock_method);
		sensitive << clk_g;

		for (i = 0; i < CORES_PER_PCH; i++) {
			dut[i] = new softsimd_pu(sc_gen_unique_name("IMCcoreUnderTest"), "softsimd_pu");
			dut[i]->clk(clk);
			dut[i]->rst(rst);
			dut[i]->RD(RD);
			dut[i]->WR(WR);
			dut[i]->ACT(ACT);
	//		dut[i]->RSTB(RSTB);
			dut[i]->AB_mode(AB_mode);
			dut[i]->pim_mode(pim_mode);
			dut[i]->bank_addr(bank_addr);
			dut[i]->row_addr(row_addr);
			dut[i]->col_addr(col_addr);
			dut[i]->DQ(DQ);
#if DUAL_BANK_INTERFACE
			dut[i]->even_in(even_in[i]);
			dut[i]->odd_in(odd_in[i]);
			dut[i]->even_out(even_out[i]);
			dut[i]->odd_out(odd_out[i]);
#else
			dut[i]->dram_in(dram_in[i]);
			dut[i]->dram_out(dram_out[i]);
#endif
		}

	    driver.rst(rst);
	    driver.RD(RD);
	    driver.WR(WR);
	    driver.ACT(ACT);
	//    driver.RSTB(RSTB);
	    driver.AB_mode(AB_mode);
	    driver.pim_mode(pim_mode);
	    driver.bank_addr(bank_addr);
	    driver.row_addr(row_addr);
	    driver.col_addr(col_addr);
	    driver.DQ(DQ);
	    for (i = 0; i < CORES_PER_PCH; i++) {
#if DUAL_BANK_INTERFACE
	        driver.even_in[i](even_in[i]);
	        driver.odd_in[i](odd_in[i]);
			driver.even_out[i](even_out[i]);
			driver.odd_out[i](odd_out[i]);
#else
			driver.dram_in[i](dram_in[i]);
			driver.dram_out[i](dram_out[i]);
#endif
	    }

	    monitor.clk(clk);
	    monitor.rst(rst);
	    monitor.RD(RD);
	    monitor.WR(WR);
	    monitor.ACT(ACT);
	//    monitor.RSTB(RSTB);
	    monitor.AB_mode(AB_mode);
	    monitor.pim_mode(pim_mode);
	    monitor.bank_addr(bank_addr);
	    monitor.row_addr(row_addr);
	    monitor.col_addr(col_addr);
	    monitor.DQ(DQ);
	    for (i = 0; i < CORES_PER_PCH; i++) {
#if DUAL_BANK_INTERFACE
	        monitor.even_in[i](even_in[i]);
	        monitor.odd_in[i](odd_in[i]);
	        monitor.even_out[i](even_out[i]);
	        monitor.odd_out[i](odd_out[i]);
#else
			monitor.dram_in[i](dram_in[i]);
			monitor.dram_out[i](dram_out[i]);
#endif
	    }

	}

	void clock_method();	// Turns bool clk_g into sc_logic clk

#else	// MIXED_SIM

	sc_clock                        clk;
	sc_signal<bool>                 rst;
	sc_signal<bool>                 RD;							// DRAM read command
	sc_signal<bool>                 WR;							// DRAM write command
	sc_signal<bool>                 ACT;						// DRAM activate command
//    sc_signal<bool>					RSTB;						//
	sc_signal<bool>                 AB_mode;		            // Signals if the All-Banks mode is enabled
	sc_signal<bool>                 pim_mode;			        // Signals if the PIM mode is enabled
	sc_signal<sc_uint<BANK_BITS> >  bank_addr;			        // Address of the bank
	sc_signal<sc_uint<ROW_BITS> >   row_addr;			        // Address of the bank row
	sc_signal<sc_uint<COL_BITS> >   col_addr;		            // Address of the bank column
	sc_signal<uint64_t>             DQ;	                        // Data input from DRAM controller (output makes no sense
#if DUAL_BANK_INTERFACE
	sc_signal_rv<GRF_WIDTH>         even_buses[CORES_PER_PCH];  // Direct data in/out to the even bank
	sc_signal_rv<GRF_WIDTH>         odd_buses[CORES_PER_PCH];   // Direct data in/out to the odd bank
#else
	sc_signal_rv<DRAM_BITS>         dram_buses[CORES_PER_PCH];  // Direct data in/out to the banks
#endif

	imc_pch dut;
	pch_driver driver;
	pch_monitor monitor;

	SC_CTOR(top) : clk("clk", CLK_PERIOD, RESOLUTION), dut("IMCpChUnderTest"),
					driver("Driver" ,"dpC32R8V100n100"), monitor("Monitor")
	{
		uint i;

		dut.clk(clk);
		dut.rst(rst);
		dut.RD(RD);
		dut.WR(WR);
		dut.ACT(ACT);
	//    dut.RSTB(RSTB);
		dut.AB_mode(AB_mode);
		dut.pim_mode(pim_mode);
		dut.bank_addr(bank_addr);
		dut.row_addr(row_addr);
		dut.col_addr(col_addr);
		dut.DQ(DQ);
		for (i = 0; i < CORES_PER_PCH; i++) {
#if DUAL_BANK_INTERFACE
			dut.even_buses[i](even_buses[i]);
			dut.odd_buses[i](odd_buses[i]);
#else
			dut.dram_buses[i](dram_buses[i]);
#endif
		}

	    driver.rst(rst);
	    driver.RD(RD);
	    driver.WR(WR);
	    driver.ACT(ACT);
	//    driver.RSTB(RSTB);
	    driver.AB_mode(AB_mode);
	    driver.pim_mode(pim_mode);
	    driver.bank_addr(bank_addr);
	    driver.row_addr(row_addr);
	    driver.col_addr(col_addr);
	    driver.DQ(DQ);
	    for (i = 0; i < CORES_PER_PCH; i++) {
#if DUAL_BANK_INTERFACE
	        driver.even_buses[i](even_buses[i]);
	        driver.odd_buses[i](odd_buses[i]);
#else
	        driver.dram_buses[i](dram_buses[i]);
#endif
	    }

	    monitor.clk(clk);
	    monitor.rst(rst);
	    monitor.RD(RD);
	    monitor.WR(WR);
	    monitor.ACT(ACT);
	//    monitor.RSTB(RSTB);
	    monitor.AB_mode(AB_mode);
	    monitor.pim_mode(pim_mode);
	    monitor.bank_addr(bank_addr);
	    monitor.row_addr(row_addr);
	    monitor.col_addr(col_addr);
	    monitor.DQ(DQ);
	    for (i = 0; i < CORES_PER_PCH; i++) {
#if DUAL_BANK_INTERFACE
	        monitor.even_buses[i](even_buses[i]);
	        monitor.odd_buses[i](odd_buses[i]);
#else
			monitor.dram_buses[i](dram_buses[i]);
#endif
	    }
	}

#endif	// MIXED_SIM

};

#endif

#endif /* PCH_MAIN_H_ */
