#ifndef PC_MAIN_H_
#define PC_MAIN_H_

#include "cp_driver.h"
#include <string>

#if MIXED_SIM
#include "../cp_wrapped.h"
#else
#include "../control_plane.h"
#endif

// #if MIXED_SIM

// #ifdef MTI_SYSTEMC

SC_MODULE(top) {

    sc_clock                        clk_g;
    sc_signal<sc_logic>             clk;
    sc_signal<sc_logic>             rst;
    sc_signal<sc_logic>             RD;                         // DRAM read command
    sc_signal<sc_logic>             WR;                         // DRAM write command
    sc_signal<sc_logic>             ACT;                        // DRAM activate command
//    sc_signal<sc_logic>             RSTB;                      //
    sc_signal<sc_logic>             AB_mode;                    // Signals if the All-Banks mode is enabled
    sc_signal<sc_logic>             pim_mode;                   // Signals if the PIM mode is enabled
    sc_signal<sc_lv<BANK_BITS> >    bank_addr;                  // Address of the bank
    sc_signal<sc_lv<ROW_BITS> >     row_addr;                   // Address of the bank row
    sc_signal<sc_lv<COL_BITS> >     col_addr;                   // Address of the bank column
    sc_signal<sc_lv<64> >           DQ;                         // Data input from DRAM controller (output makes no sense
    sc_signal<sc_lv<64> >           data_out;                   // Data to the Registers

    // Tile shuffler control
    sc_signal<sc_logic>     ts_in_en;       // Enable input to tile shuffle
    sc_signal<sc_logic>     ts_out_en;      // Enable output from tile shuffle
    sc_signal<sc_lv<32> >   ts_out_start;   // Start location for the words shuffled out
    sc_signal<sc_lv<32> >   ts_out_mode;    // Shuffling mode
    sc_signal<sc_lv<32> >   ts_shf_from;    // Index the MUX for input data

    // VWRs control
    sc_signal<sc_logic>     vwr_enable[VWR_NUM];    // Enable signals for the VWRs
    sc_signal<sc_logic>     vwr_wr_nrd[VWR_NUM];    // Read/write signals for the VWRs
#if VWR_DRAM_CLK > 1
    sc_signal<sc_logic>     vwr_dram_d_nm[VWR_NUM]; // Multiplex/demultiplex signals for the VWRs-DRAM interface
    sc_signal<sc_lv<32> >   vwr_dram_idx;           // Index of the VWRs-DRAM mux/demux
#endif
    sc_signal<sc_logic>     vwr_d_nm[VWR_NUM];      // Multiplex/demultiplex signals for the VWRs
    sc_signal<sc_logic>     vwr_mask_en[VWR_NUM];   // Enable VWR masked access
    sc_signal<sc_lv<64> >   vwr_mask[VWR_NUM];      // Word mask for VWR accesses to and from DRAM
    sc_signal<sc_lv<32> >   vwr_idx[VWR_NUM];       // Word indeces of the VWR accesses
    sc_signal<sc_lv<32> >   vwr_from[VWR_NUM];      // Index the MUX for input data

    // Registers control
    sc_signal<sc_logic>     reg_en[REG_NUM];    // Write enable for the registers in the datapath
    sc_signal<sc_lv<32> >   reg_from[REG_NUM];  // Index the MUX for input data

    // SRF control
    sc_signal<sc_lv<32> >   srf_rd_addr;    // Index read
    sc_signal<sc_logic>     srf_wr_en;      // Enable writing
    sc_signal<sc_lv<32> >   srf_wr_addr;    // Index the address to be written
    sc_signal<sc_lv<32> >   srf_wr_from;    // Index the MUX for input data

    // Mask RF control
    sc_signal<sc_lv<32> >   mrf_rd_addr;    // Index read
    sc_signal<sc_logic>     mrf_wr_en;      // Enable writing
    sc_signal<sc_lv<32> >   mrf_wr_addr;    // Index the address to be written
    sc_signal<sc_lv<32> >   mrf_wr_from;    // Index the MUX for input data

    // Shift and Add control
    sc_signal<sc_logic>     sa_en;          // Enables shift & add stage
    sc_signal<sc_lv<32> >   sa_shift;       // Specifies shift
    sc_signal<sc_lv<32> >   sa_size;        // Specifies subword size
    sc_signal<sc_logic>     sa_adder_en;    // Enables adder/subtractor
    sc_signal<sc_logic>     sa_neg_op1;     // Signals if subtracting op1
    sc_signal<sc_logic>     sa_neg_op2;     // Signals if subtracting op2
    sc_signal<sc_lv<32> >   sa_op1_from;    // Index MUX for input data 1
    sc_signal<sc_lv<32> >   sa_op2_from;    // Index MUX for input data 2

    // Pack and Mask control
    sc_signal<sc_logic>     pm_en;          // Enables pack & mask stage
    sc_signal<sc_lv<32> >   pm_repack;      // Control for the repacker
    sc_signal<sc_lv<32> >   pm_in_start;    // Position of subword in w1+w2 where packing to output starts
    sc_signal<sc_lv<32> >   pm_op_sel;      // Selection of mask operation
    sc_signal<sc_lv<32> >   pm_shift;       // Positions to shift right
    sc_signal<sc_lv<32> >   pm_op1_from;    // Index MUX for input data 1
    sc_signal<sc_lv<32> >   pm_op2_from;    // Index MUX for input data 2

#if DUAL_BANK_INTERFACE
    // BANKS Control
    sc_signal<sc_logic>     even_out_en;   // Enables the even bank tri-state buffer
    sc_signal<sc_logic>     odd_out_en;    // Enables the odd bank tri-state buffer
#else
    sc_signal<sc_logic>     dram_out_en;    // Enables the DRAM tri-state buffer
    sc_signal<sc_lv<32> >   dram_from;      // Index the MUX for input data
#endif

    control_plane dut;
    cp_driver driver;

    SC_CTOR(top) : clk_g("clk", CLK_PERIOD, RESOLUTION), dut("ControlPlaneUnderTest", "control_plane"),
                    driver("Driver")
    {
        uint i;

        SC_METHOD(clock_method);
        sensitive << clk_g;

        dut.clk(clk);
        dut.rst(rst);
        dut.RD(RD);
        dut.WR(WR);
        dut.ACT(ACT);
//        dut.RSTB(RSTB);
        dut.AB_mode(AB_mode);
        dut.pim_mode(pim_mode);
        dut.bank_addr(bank_addr);
        dut.row_addr(row_addr);
        dut.col_addr(col_addr);
        dut.DQ(DQ);
        dut.data_out(data_out);
        dut.ts_in_en(ts_in_en);
        dut.ts_out_en(ts_out_en);
        dut.ts_out_start(ts_out_start);
        dut.ts_out_mode(ts_out_mode);
        dut.ts_shf_from(ts_shf_from);
        dut.vwr_enable_0(vwr_enable[0]);
        dut.vwr_enable_1(vwr_enable[1]);
        dut.vwr_wr_nrd_0(vwr_wr_nrd[0]);
        dut.vwr_wr_nrd_1(vwr_wr_nrd[1]);
#if VWR_DRAM_CLK > 1
        dut.vwr_dram_d_nm_0(vwr_dram_d_nm[0]);
        dut.vwr_dram_d_nm_1(vwr_dram_d_nm[1]);
#endif
        dut.vwr_d_nm_0(vwr_d_nm[0]);
        dut.vwr_d_nm_1(vwr_d_nm[1]);
        dut.vwr_mask_en_0(vwr_mask_en[0]);
        dut.vwr_mask_en_1(vwr_mask_en[1]);
        dut.vwr_mask_0(vwr_mask[0]);
        dut.vwr_mask_1(vwr_mask[1]);
        dut.vwr_idx_0(vwr_idx[0]);
        dut.vwr_idx_1(vwr_idx[1]);
        dut.vwr_from_0(vwr_from[0]);
        dut.vwr_from_1(vwr_from[1]);
#if VWR_DRAM_CLK > 1
        dut.vwr_dram_idx(vwr_dram_idx);
#endif
        dut.reg_en_0(reg_en[0]);
        dut.reg_en_1(reg_en[1]);
        dut.reg_en_2(reg_en[2]);
        dut.reg_en_3(reg_en[3]);
        dut.reg_from_0(reg_from[0]);
        dut.reg_from_1(reg_from[1]);
        dut.reg_from_2(reg_from[2]);
        dut.reg_from_3(reg_from[3]);
        dut.srf_rd_addr(srf_rd_addr);
        dut.srf_wr_en(srf_wr_en);
        dut.srf_wr_addr(srf_wr_addr);
        dut.srf_wr_from(srf_wr_from);
        dut.mrf_rd_addr(mrf_rd_addr);
        dut.mrf_wr_en(mrf_wr_en);
        dut.mrf_wr_addr(mrf_wr_addr);
        dut.mrf_wr_from(mrf_wr_from);
        dut.sa_en(sa_en);
        dut.sa_shift(sa_shift);
        dut.sa_size(sa_size);
        dut.sa_adder_en(sa_adder_en);
        dut.sa_neg_op1(sa_neg_op1);
        dut.sa_neg_op2(sa_neg_op2);
        dut.sa_op1_from(sa_op1_from);
        dut.sa_op2_from(sa_op2_from);
        dut.pm_en(pm_en);
        dut.pm_repack(pm_repack);
        dut.pm_in_start(pm_in_start);
        dut.pm_op_sel(pm_op_sel);
        dut.pm_shift(pm_shift);
        dut.pm_op1_from(pm_op1_from);
        dut.pm_op2_from(pm_op2_from);
#if DUAL_BANK_INTERFACE
        dut.even_out_en(even_out_en);
        dut.odd_out_en(odd_out_en);
#else
        dut.dram_out_en(dram_out_en);
        dut.dram_from(dram_from);
#endif

        driver.rst(rst);
        driver.RD(RD);
        driver.WR(WR);
        driver.ACT(ACT);
//        driver.RSTB(RSTB);
        driver.AB_mode(AB_mode);
        driver.pim_mode(pim_mode);
        driver.bank_addr(bank_addr);
        driver.row_addr(row_addr);
        driver.col_addr(col_addr);
        driver.DQ(DQ);
    }

    void clock_method();    // Turns bool clk_g into sc_logic clk

};

// #endif // MTI_SYSTEMC

// #endif // MIXED_SIM

#endif /* PC_MAIN_H_ */