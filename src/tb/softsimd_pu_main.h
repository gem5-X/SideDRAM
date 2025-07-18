#ifndef SOFTSIMD_PU_MAIN_H_
#define SOFTSIMD_PU_MAIN_H_

#include <string>
#include "softsimd_pu_driver.h"
#include "softsimd_pu_monitor.h"

#if MIXED_SIM
#include "../softsimd_pu_cu_test.h"
#else
#include "../softsimd_pu.h"
#endif

#ifdef MTI_SYSTEMC

SC_MODULE(top) {

    sc_clock                        clk;
    sc_signal<bool>                 rst;
    sc_signal<bool>                 RD;         // DRAM read command
    sc_signal<bool>                 WR;         // DRAM write command
    sc_signal<bool>                 ACT;        // DRAM activate command
//    sc_signal<bool>                   RSTB;       //
    sc_signal<bool>                 AB_mode;    // Signals if the All-Banks mode is enabled
    sc_signal<bool>                 pim_mode;   // Signals if the PIM mode is enabled
    sc_signal<sc_uint<BANK_BITS> >  bank_addr;  // Address of the bank
    sc_signal<sc_uint<ROW_BITS> >   row_addr;   // Address of the bank row
    sc_signal<sc_uint<COL_BITS> >   col_addr;   // Address of the bank column
    sc_signal<uint64_t>             DQ;         // Data input from DRAM controller (output makes no sense
#if DUAL_BANK_INTERFACE
    sc_signal_rv<DRAM_BITS>         even_bus;   // Direct data in/out to the even bank
    sc_signal_rv<DRAM_BITS>         odd_bus;    // Direct data in/out to the odd bank
#else
    sc_signal_rv<DRAM_BITS>         dram_bus;   // Direct data in/out to the DRAM
#endif

    softsimd_pu dut;
    softsimd_pu_driver driver;
    softsimd_pu_monitor monitor;

    SC_CTOR(top) : clk("clk", CLK_PERIOD, RESOLUTION), dut("SoftSIMDCoreUnderTest"),
                    driver("Driver"), monitor("Monitor")
    {
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
#if DUAL_BANK_INTERFACE
        dut.even_bus(even_bus);
        dut.odd_bus(odd_bus);
#else
        dut.dram_bus(dram_bus);
#endif

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
#if DUAL_BANK_INTERFACE
        driver.even_bus(even_bus);
        driver.odd_bus(odd_bus);
#else
        driver.dram_bus(dram_bus);
#endif

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
#if DUAL_BANK_INTERFACE
        monitor.even_bus(even_bus);
        monitor.odd_bus(odd_bus);
#else
        monitor.dram_bus(dram_bus);
#endif
    }

};

#endif

#endif /* SOFTSIMD_PU_MAIN_H_ */
