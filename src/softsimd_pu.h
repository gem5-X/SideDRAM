/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the SoftSIMD Processing Unit.
 *
 */

#include "cnm_base.h"

#if !MIXED_SIM

#ifndef SRC_SOFTSIMD_PU_H_
#define SRC_SOFTSIMD_PU_H_

#include "control_unit.h"
#include "pack_and_mask.h"
#include "register.h"
#include "rf_twoport.h"
#include "shift_and_add.h"
#include "tile_shuffler.h"
#include "tristate_buffer.h"
#include "vwr.h"

#if RECORDING
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>

#define M7_SET  0x80
#define M6_SET  0x40
#define M5_SET  0x20
#define M4_SET  0x10
#define M3_SET  0x08
#define M2_SET  0x04
#define M1_SET  0x02
#define M0_SET  0x01
#endif

class softsimd_pu: public sc_module {
public:

#ifndef __SYNTHESIS__

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

    // ** INTERNAL SIGNALS AND VARIABLES **
    // Basic control
    sc_signal<uint64_t> macroinstr;
    sc_signal<uint>     PC;
    sc_signal<uint64_t> cu_data_out;
#if DUAL_BANK_INTERFACE
    sc_signal<bool>     even_out_en, odd_out_en;
#else
    sc_signal<bool>                 dram_out_en;
    sc_signal<MUX>                  dram_from;
    sc_signal<sc_lv<DRAM_BITS> >    to_dram;
#endif
    // Instruction buffer
    sc_signal<bool>     ib_wr_en;
    sc_signal<uint>     ib_wr_addr;
    sc_signal<uint64_t> ib_in;
    // Tile shuffler
    sc_signal<bool>         ts_in_en, ts_out_en;
    sc_signal<uint>         ts_out_start;
    sc_signal<TS_MODE>      ts_mode;
    sc_signal<MUX>          ts_shf_from;
    sc_signal_rv<VWR_BITS>  ts_inout;
    // VWRs
    sc_signal<bool>         vwr_enable[VWR_NUM], vwr_wr_nrd[VWR_NUM], vwr_d_nm[VWR_NUM];
    sc_signal<uint>         vwr_idx[VWR_NUM];
    sc_signal<bool>         vwr_mask_en[VWR_NUM];
    sc_signal<uint64_t>     vwr_mask[VWR_NUM];
    sc_signal<MUX>          vwr_from[VWR_NUM];
    sc_signal_rv<VWR_BITS>  vwr_inout[VWR_NUM];
    sc_signal_rv<WORD_BITS> vwr_muxed[VWR_NUM];
#if VWR_DRAM_CLK > 1
    sc_signal<bool>         vwr_dram_d_nm[VWR_NUM];
    sc_signal<uint>         vwr_dram_idx;
    sc_signal_rv<DRAM_BITS> vwr_dram_if[VWR_NUM];
#endif
    // Registers
    sc_signal<bool>     reg_en[REG_NUM];
    sc_signal<MUX>      reg_from[REG_NUM];
    sc_signal<uint64_t> R_in[REG_NUM][WORD_64B], R_out[REG_NUM][WORD_64B];
    // SRF
    sc_signal<uint>     srf_rd_addr, srf_wr_addr;
    sc_signal<bool>     srf_wr_en;
    sc_signal<MUX>      srf_wr_from;
    sc_signal<uint64_t> srf_in[WORD_64B], srf_out[WORD_64B];
    // Mask RF
    sc_signal<uint>     mrf_rd_addr, mrf_wr_addr;
    sc_signal<bool>     mrf_wr_en;
    sc_signal<MUX>      mrf_wr_from;
    sc_signal<uint64_t> mrf_in[WORD_64B], mrf_out[WORD_64B];
    // CSD RF
    sc_signal<uint>     csdrf_rd_addr, csdrf_wr_addr;
    sc_signal<bool>     csdrf_wr_en;
    sc_signal<MUX>      csdrf_wr_from;
    sc_signal<uint64_t> csdrf_in[WORD_64B], csdrf_out[WORD_64B];
    // Shift & Add stage
    sc_signal<bool>     sa_en, sa_adder_en, sa_neg_op1, sa_neg_op2;
    sc_signal<uint>     sa_shift;
    sc_signal<SWSIZE>   sa_size;
    sc_signal<MUX>      sa_op1_from, sa_op2_from;
    sc_signal<uint64_t> sa_op1[WORD_64B], sa_op2[WORD_64B], sa_out[WORD_64B];
    // Pack & Mask stage
    sc_signal<bool>     pm_en;
    sc_signal<SWREPACK> pm_repack;
    sc_signal<uint>     pm_in_start, pm_shift;
    sc_signal<MASKOP>   pm_op_sel;
    sc_signal<MUX>      pm_op1_from, pm_op2_from;
    sc_signal<uint64_t> pm_w1[WORD_64B], pm_w2[WORD_64B], pm_out[WORD_64B];

    // Internal modules
    control_unit *cu;
#if (EN_MODEL == 0) // Skip for fast energy model generation
    shift_and_add *sa_stage;
    pack_and_mask *pm_stage;
    tile_shuffler *ts;
#endif
    rf_twoport<uint64_t,IB_ENTRIES> *ib_macro;
    rf_twoport<uint64_t, CSD_ENTRIES> *csdrf[CSD_64B];
    rf_twoport<uint64_t, SRF_ENTRIES> *srf[MASK_64B];
    rf_twoport<uint64_t, MASK_ENTRIES> *mrf[MASK_64B];
    reg<WORD_64B> *R[REG_NUM];
#if VWR_DRAM_CLK > 1
    vwr_multicycle<VWR_BITS, WORD_BITS, DRAM_BITS> *vwreg[VWR_NUM];
#else
    vwr<VWR_BITS, WORD_BITS, WORDS_PER_VWR> *vwreg[VWR_NUM];
#endif
#if DUAL_BANK_INTERFACE
    tristate_buffer<DRAM_BITS> *even_buf, *odd_buf;
#else
    tristate_buffer<DRAM_BITS> *dram_buf;
#endif

    SC_HAS_PROCESS(softsimd_pu);
#if RECORDING
    ofstream record_file;
#endif
#if (RECORDING || EN_MODEL)
    std::string filename;
    softsimd_pu(sc_module_name name, std::string filename_) : sc_module(name), filename(filename_) {
#else
    softsimd_pu(sc_module_name name) : sc_module(name) {
#endif
        int i, j;

#if EN_MODEL
        cu = new control_unit("control_unit", filename);
#else
        cu = new control_unit("control_unit");
#endif
        cu->clk(clk);
        cu->rst(rst);
        cu->RD(RD);
        cu->WR(WR);
        cu->ACT(ACT);
//      cu->RSTB(RSTB);
        cu->AB_mode(AB_mode);
        cu->pim_mode(pim_mode);
        cu->bank_addr(bank_addr);
        cu->row_addr(row_addr);
        cu->col_addr(col_addr);
        cu->DQ(DQ);
        cu->macroinstr(macroinstr);
        for (i=0; i<CSD_64B; i++)
            cu->csd_mult[i](csdrf_out[i]);
        cu->pc_out(PC);
        cu->data_out(cu_data_out);
        // Instruction buffer control
        cu->ib_wr_en(ib_wr_en);
        cu->ib_wr_addr(ib_wr_addr);
        // Tile shuffler control
        cu->ts_in_en(ts_in_en);
        cu->ts_out_en(ts_out_en);
        cu->ts_out_start(ts_out_start);
        cu->ts_out_mode(ts_mode);
        cu->ts_shf_from(ts_shf_from);
        // VWRs control
        for (i=0; i<VWR_NUM; i++) {
            cu->vwr_enable[i](vwr_enable[i]);
            cu->vwr_wr_nrd[i](vwr_wr_nrd[i]);
            cu->vwr_d_nm[i](vwr_d_nm[i]);
#if VWR_DRAM_CLK > 1
            cu->vwr_dram_d_nm[i](vwr_dram_d_nm[i]);
#endif
            cu->vwr_mask_en[i](vwr_mask_en[i]);
            cu->vwr_mask[i](vwr_mask[i]);
            cu->vwr_idx[i](vwr_idx[i]);
            cu->vwr_from[i](vwr_from[i]);
        }
#if VWR_DRAM_CLK > 1
        cu->vwr_dram_idx(vwr_dram_idx);
#endif
        // Registers control
        for (i=0; i<REG_NUM; i++) {
            cu->reg_en[i](reg_en[i]);
            cu->reg_from[i](reg_from[i]);
        }
        // SRF control
        cu->srf_rd_addr(srf_rd_addr);
        cu->srf_wr_en(srf_wr_en);
        cu->srf_wr_addr(srf_wr_addr);
        cu->srf_wr_from(srf_wr_from);
        // Mask RF control
        cu->mrf_rd_addr(mrf_rd_addr);
        cu->mrf_wr_en(mrf_wr_en);
        cu->mrf_wr_addr(mrf_wr_addr);
        cu->mrf_wr_from(mrf_wr_from);
        // CSD RF control
        cu->csdrf_rd_addr(csdrf_rd_addr);
        cu->csdrf_wr_en(csdrf_wr_en);
        cu->csdrf_wr_addr(csdrf_wr_addr);
        cu->csdrf_wr_from(csdrf_wr_from);
        // Shift and Add control
        cu->sa_en(sa_en);
        cu->sa_shift(sa_shift);
        cu->sa_size(sa_size);
        cu->sa_adder_en(sa_adder_en);
        cu->sa_neg_op1(sa_neg_op1);
        cu->sa_neg_op2(sa_neg_op2);
        cu->sa_op1_from(sa_op1_from);
        cu->sa_op2_from(sa_op2_from);
        // Pack and Mask control
        cu->pm_en(pm_en);
        cu->pm_repack(pm_repack);
        cu->pm_in_start(pm_in_start);
        cu->pm_op_sel(pm_op_sel);
        cu->pm_shift(pm_shift);
        cu->pm_op1_from(pm_op1_from);
        cu->pm_op2_from(pm_op2_from);
#if DUAL_BANK_INTERFACE
        // BANKS Control
        cu->even_out_en(even_out_en);
        cu->odd_out_en(odd_out_en);
#else
        cu->dram_out_en(dram_out_en);
        cu->dram_from(dram_from);
#endif

#if (EN_MODEL == 0) // Skip for fast energy model generation
        sa_stage = new shift_and_add("Shift&Add");
        sa_stage->clk(clk);
        sa_stage->rst(rst);
        sa_stage->compute_en(sa_en);
        sa_stage->shift(sa_shift);
        sa_stage->size(sa_size);
        sa_stage->adder_en(sa_adder_en);
        sa_stage->neg_op1(sa_neg_op1);
        sa_stage->neg_op2(sa_neg_op2);
        for (i=0; i<WORD_64B; i++) {
            sa_stage->op1[i](sa_op1[i]);
            sa_stage->op2[i](sa_op2[i]);
            sa_stage->output[i](sa_out[i]);
        }

        pm_stage = new pack_and_mask("Pack&Mask");
        pm_stage->clk(clk);
        pm_stage->rst(rst);
        pm_stage->compute_en(pm_en);
        for (i=0; i<WORD_64B; i++) {
            pm_stage->w1[i](pm_w1[i]);
            pm_stage->w2[i](pm_w2[i]);
        }
        pm_stage->repack(pm_repack);
        pm_stage->in_start(pm_in_start);
        for (i=0; i<MASK_64B; i++)
            pm_stage->mask_in[i](mrf_out[i]);
        pm_stage->op_sel(pm_op_sel);
        pm_stage->shift(pm_shift);
        for (i=0; i<WORD_64B; i++)
            pm_stage->output[i](pm_out[i]);

        ts = new tile_shuffler("Tile_shuffler");
        ts->clk(clk);
        ts->rst(rst);
        ts->input_en(ts_in_en);
        ts->output_en(ts_out_en);
        ts->out_start(ts_out_start);
        ts->mode(ts_mode);
        ts->vww_inout(ts_inout);
#endif

        ib_macro = new rf_twoport<uint64_t, IB_ENTRIES>("Macroinstruction_buffer");
        ib_macro->clk(clk);
        ib_macro->rst(rst);
        ib_macro->rd_addr(PC);
        ib_macro->rd_port(macroinstr);
        ib_macro->wr_en(ib_wr_en);
        ib_macro->wr_addr(ib_wr_addr);
        ib_macro->wr_port(ib_in);

        for (i=0; i<MASK_64B; i++) {
            srf[i] = new rf_twoport<uint64_t, SRF_ENTRIES>(sc_gen_unique_name("SRF"));
            srf[i]->clk(clk);
            srf[i]->rst(rst);
            srf[i]->rd_addr(srf_rd_addr);
            srf[i]->rd_port(srf_out[i]);
            srf[i]->wr_en(srf_wr_en);
            srf[i]->wr_addr(srf_wr_addr);
            srf[i]->wr_port(srf_in[i]);
        }

        for (i=0; i<MASK_64B; i++) {
            mrf[i] = new rf_twoport<uint64_t, MASK_ENTRIES>(sc_gen_unique_name("MRF"));
            mrf[i]->clk(clk);
            mrf[i]->rst(rst);
            mrf[i]->rd_addr(mrf_rd_addr);
            mrf[i]->rd_port(mrf_out[i]);
            mrf[i]->wr_en(mrf_wr_en);
            mrf[i]->wr_addr(mrf_wr_addr);
            mrf[i]->wr_port(mrf_in[i]);
        }

        for (i=0; i<CSD_64B; i++) {
            csdrf[i] = new rf_twoport<uint64_t, CSD_ENTRIES>(sc_gen_unique_name("CSD_RF"));
            csdrf[i]->clk(clk);
            csdrf[i]->rst(rst);
            csdrf[i]->rd_addr(csdrf_rd_addr);
            csdrf[i]->rd_port(csdrf_out[i]);
            csdrf[i]->wr_en(csdrf_wr_en);
            csdrf[i]->wr_addr(csdrf_wr_addr);
            csdrf[i]->wr_port(csdrf_in[i]);
        }

        for (i=0; i<REG_NUM; i++) {
            R[i] = new reg<WORD_64B>(sc_gen_unique_name("R"));
            R[i]->clk(clk);
            R[i]->rst(rst);
            R[i]->en(reg_en[i]);
            for (j=0; j<WORD_64B; j++) {
                R[i]->input[j](R_in[i][j]);
                R[i]->output[j](R_out[i][j]);
            }
        }

#if VWR_DRAM_CLK > 1
        for (i=0; i<VWR_NUM; i++) {
            vwreg[i] = new vwr_multicycle<VWR_BITS, WORD_BITS, DRAM_BITS>(sc_gen_unique_name("VWR"));
            vwreg[i]->clk(clk);
            vwreg[i]->rst(rst);
            vwreg[i]->enable(vwr_enable[i]);
            vwreg[i]->wr_nrd(vwr_wr_nrd[i]);
            vwreg[i]->dram_idx(vwr_dram_idx);
            vwreg[i]->dram_d_nm(vwr_dram_d_nm[i]);
            vwreg[i]->idx(vwr_idx[i]);
            vwreg[i]->d_nm(vwr_d_nm[i]);
            vwreg[i]->lport(vwr_inout[i]);
            vwreg[i]->sport(vwr_muxed[i]);
            vwreg[i]->dport(vwr_dram_if[i]);
        }
#else
        for (i=0; i<VWR_NUM; i++) {
            vwreg[i] = new vwr<VWR_BITS, WORD_BITS, WORDS_PER_VWR>(sc_gen_unique_name("VWR"));
            vwreg[i]->clk(clk);
            vwreg[i]->rst(rst);
            vwreg[i]->enable(vwr_enable[i]);
            vwreg[i]->wr_nrd(vwr_wr_nrd[i]);
            vwreg[i]->idx(vwr_idx[i]);
            vwreg[i]->mask_en(vwr_mask_en[i]);
            vwreg[i]->mask(vwr_mask[i]);
            vwreg[i]->d_nm(vwr_d_nm[i]);
            vwreg[i]->lport(vwr_inout[i]);
            vwreg[i]->sport(vwr_muxed[i]);
        }
#endif

#if DUAL_BANK_INTERFACE

        even_buf = new tristate_buffer<DRAM_BITS>("Even_tristate_buffer");
        odd_buf = new tristate_buffer<DRAM_BITS>("Odd_tristate_buffer");
#if VWR_DRAM_CLK > 1
        even_buf->input(vwr_dram_if[0]);
        odd_buf->input(vwr_dram_if[1]);
#else   // VWR_DRAM_CLK > 1
        even_buf->input(vwr_inout[0]);
        odd_buf->input(vwr_inout[1]);
#endif  // VWR_DRAM_CLK > 1
        even_buf->enable(even_out_en);
        odd_buf->enable(odd_out_en);
        even_buf->output(even_bus);
        odd_buf->output(odd_bus);

#else   // DUAL_BANK_INTERFACE

        dram_buf = new tristate_buffer<DRAM_BITS>("DRAM_tristate_buffer");
        dram_buf->input(to_dram);
        dram_buf->enable(dram_out_en);
        dram_buf->output(dram_bus);

#endif  // DUAL_BANK_INTERFACE

        SC_METHOD(comb_method);
        sensitive << ts_shf_from << ts_inout;
        sensitive << srf_wr_from << mrf_wr_from << csdrf_wr_from;
        sensitive << sa_op1_from << sa_op2_from << pm_op1_from << pm_op2_from;
        for (int i=0; i<MASK_64B; i++)
            sensitive << srf_out[i] << mrf_out[i];
        for (int i=0; i<WORD_64B; i++)
            sensitive << sa_out[i] << pm_out[i] << cu_data_out;
        for (int i=0; i<REG_NUM; i++) {
            sensitive << reg_from[i];
            for (int j=0; j<WORD_64B; j++)
                sensitive << R_out[i][j];
        }
        for (int i=0; i<VWR_NUM; i++)
            sensitive << vwr_wr_nrd[i] << vwr_from[i] << vwr_inout[i] << vwr_muxed[i];
#if DUAL_BANK_INTERFACE
        sensitive << even_bus << odd_bus;
#else
        sensitive << dram_bus << dram_from;
#endif

        SC_METHOD(adaptation_method);
        for (int i=0; i<VWR_64B; i++)
            sensitive << cu_data_out;

#if RECORDING
        // Open record file
        std::string fo = "inputs/recording/" + filename + ".rec";
        record_file.open(fo);
        if (!record_file.is_open()) {
            cout << "Error when opening output file" << endl;
            sc_stop();
            return;
        }

        SC_THREAD(record_thread);
        sensitive << clk.pos();
#endif
    }

    void comb_method();
    void adaptation_method();   // Adapts C types to SystemC types
#if RECORDING
    void record_thread();       // Records control signals and VWRs inout
    void swlen_to_mask(SWSIZE swlen, uint64_t *mask);
    uint shuffler_control(SWREPACK repack, uint in_start);
#endif

#else   // __SYNTHESIS__

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
    sc_in<sc_lv<DRAM_BITS> >    even_in;    // Direct data in/out to the even bank
    sc_in<sc_lv<DRAM_BITS> >    odd_in;     // Direct data in/out to the odd bank
    sc_out<sc_lv<DRAM_BITS> >   even_out;   // Direct data in/out to the even bank
    sc_out<sc_lv<DRAM_BITS> >   odd_out;    // Direct data in/out to the odd bank
#else   // DUAL_BANK_INTERFACE
    sc_in<sc_lv<DRAM_BITS> >    dram_in;    // Direct data in/out to the bank
    sc_out<sc_lv<DRAM_BITS> >   dram_out;   // Direct data in/out to the bank
#endif  // DUAL_BANK_INTERFACE

    // ** INTERNAL SIGNALS AND VARIABLES **
    // Basic control
    sc_signal<uint64_t> macroinstr;
    sc_signal<uint>     PC;
    sc_signal<uint64_t> cu_data_out;
#if DUAL_BANK_INTERFACE
    sc_signal<bool>     even_out_en, odd_out_en;
#else
    sc_signal<bool>                 dram_out_en;
    sc_signal<MUX>                  dram_from;
#endif
    // Instruction buffer
    sc_signal<bool>     ib_wr_en;
    sc_signal<uint>     ib_wr_addr;
    sc_signal<uint64_t> ib_in;
    // Tile shuffler
    sc_signal<bool>             ts_in_en, ts_out_en;
    sc_signal<uint>             ts_out_start;
    sc_signal<TS_MODE>          ts_mode;
    sc_signal<MUX>              ts_shf_from;
    sc_signal<sc_lv<VWR_BITS> > ts_in, ts_out;
    // VWRs
    sc_signal<bool>                 vwr_enable[VWR_NUM], vwr_wr_nrd[VWR_NUM], vwr_d_nm[VWR_NUM];
    sc_signal<uint>                 vwr_idx[VWR_NUM];
    sc_signal<bool>                 vwr_mask_en[VWR_NUM];
    sc_signal<uint64_t>             vwr_mask[VWR_NUM];
    sc_signal<MUX>                  vwr_from[VWR_NUM];
    sc_signal<sc_lv<VWR_BITS> >     vwr_in[VWR_NUM], vwr_out[VWR_NUM];
    sc_signal<sc_lv<WORD_BITS> >    vwr_muxed_in[VWR_NUM], vwr_muxed_out[VWR_NUM];
#if VWR_DRAM_CLK > 1
    sc_signal<bool>         vwr_dram_d_nm[VWR_NUM];
    sc_signal<uint>         vwr_dram_idx;
    sc_signal_rv<DRAM_BITS> vwr_dram_if[VWR_NUM];
#endif
    // Registers
    sc_signal<bool>     reg_en[REG_NUM];
    sc_signal<MUX>      reg_from[REG_NUM];
    sc_signal<uint64_t> R_in[REG_NUM][WORD_64B], R_out[REG_NUM][WORD_64B];
    // SRF
    sc_signal<uint>     srf_rd_addr, srf_wr_addr;
    sc_signal<bool>     srf_wr_en;
    sc_signal<MUX>      srf_wr_from;
    sc_signal<uint64_t> srf_in[WORD_64B], srf_out[WORD_64B];
    // Mask RF
    sc_signal<uint>     mrf_rd_addr, mrf_wr_addr;
    sc_signal<bool>     mrf_wr_en;
    sc_signal<MUX>      mrf_wr_from;
    sc_signal<uint64_t> mrf_in[WORD_64B], mrf_out[WORD_64B];
    // CSD RF
    sc_signal<uint>     csdrf_rd_addr, csdrf_wr_addr;
    sc_signal<bool>     csdrf_wr_en;
    sc_signal<MUX>      csdrf_wr_from;
    sc_signal<uint64_t> csdrf_in[WORD_64B], csdrf_out[WORD_64B];
    // Shift & Add stage
    sc_signal<bool>     sa_en, sa_adder_en, sa_neg_op1, sa_neg_op2;
    sc_signal<uint>     sa_shift;
    sc_signal<SWSIZE>   sa_size;
    sc_signal<MUX>      sa_op1_from, sa_op2_from;
    sc_signal<uint64_t> sa_op1[WORD_64B], sa_op2[WORD_64B], sa_out[WORD_64B];
    // Pack & Mask stage
    sc_signal<bool>     pm_en;
    sc_signal<SWREPACK> pm_repack;
    sc_signal<uint>     pm_in_start, pm_shift;
    sc_signal<MASKOP>   pm_op_sel;
    sc_signal<MUX>      pm_op1_from, pm_op2_from;
    sc_signal<uint64_t> pm_w1[WORD_64B], pm_w2[WORD_64B], pm_out[WORD_64B];

    // Internal modules
    control_unit *cu;
    shift_and_add *sa_stage;
    pack_and_mask *pm_stage;
    tile_shuffler *ts;
    rf_twoport<uint64_t, IB_ENTRIES> *ib_macro;
    rf_twoport<uint64_t, CSD_ENTRIES> *csdrf[CSD_64B];
    rf_twoport<uint64_t, SRF_ENTRIES> *srf[MASK_64B];
    rf_twoport<uint64_t, MASK_ENTRIES> *mrf[MASK_64B];
    reg<WORD_64B> *R[REG_NUM];
#if VWR_DRAM_CLK > 1
    vwr_multicycle<VWR_BITS, WORD_BITS, DRAM_BITS> *vwreg[VWR_NUM];
#else
    vwr<VWR_BITS, WORD_BITS, WORDS_PER_VWR> *vwreg[VWR_NUM];
#endif

    SC_HAS_PROCESS(softsimd_pu);
    softsimd_pu(sc_module_name name) : sc_module(name) {
        int i, j;

        cu = new control_unit("control_unit");
        cu->clk(clk);
        cu->rst(rst);
        cu->RD(RD);
        cu->WR(WR);
        cu->ACT(ACT);
//      cu->RSTB(RSTB);
        cu->AB_mode(AB_mode);
        cu->pim_mode(pim_mode);
        cu->bank_addr(bank_addr);
        cu->row_addr(row_addr);
        cu->col_addr(col_addr);
        cu->DQ(DQ);
        cu->macroinstr(macroinstr);
        for (i=0; i<CSD_64B; i++)
            cu->csd_mult[i](csdrf_out[i]);
        cu->pc_out(PC);
        cu->data_out(cu_data_out);
        // Instruction buffer control
        cu->ib_wr_en(ib_wr_en);
        cu->ib_wr_addr(ib_wr_addr);
        // Tile shuffler control
        cu->ts_in_en(ts_in_en);
        cu->ts_out_en(ts_out_en);
        cu->ts_out_start(ts_out_start);
        cu->ts_out_mode(ts_mode);
        cu->ts_shf_from(ts_shf_from);
        // VWRs control
        for (i=0; i<VWR_NUM; i++) {
            cu->vwr_enable[i](vwr_enable[i]);
            cu->vwr_wr_nrd[i](vwr_wr_nrd[i]);
            cu->vwr_d_nm[i](vwr_d_nm[i]);
#if VWR_DRAM_CLK > 1
            cu->vwr_dram_d_nm[i](vwr_dram_d_nm[i]);
#endif
            cu->vwr_mask_en[i](vwr_mask_en[i]);
            cu->vwr_mask[i](vwr_mask[i]);
            cu->vwr_idx[i](vwr_idx[i]);
            cu->vwr_from[i](vwr_from[i]);
        }
#if VWR_DRAM_CLK > 1
        cu->vwr_dram_idx(vwr_dram_idx);
#endif
        // Registers control
        for (i=0; i<REG_NUM; i++) {
            cu->reg_en[i](reg_en[i]);
            cu->reg_from[i](reg_from[i]);
        }
        // SRF control
        cu->srf_rd_addr(srf_rd_addr);
        cu->srf_wr_en(srf_wr_en);
        cu->srf_wr_addr(srf_wr_addr);
        cu->srf_wr_from(srf_wr_from);
        // Mask RF control
        cu->mrf_rd_addr(mrf_rd_addr);
        cu->mrf_wr_en(mrf_wr_en);
        cu->mrf_wr_addr(mrf_wr_addr);
        cu->mrf_wr_from(mrf_wr_from);
        // CSD RF control
        cu->csdrf_rd_addr(csdrf_rd_addr);
        cu->csdrf_wr_en(csdrf_wr_en);
        cu->csdrf_wr_addr(csdrf_wr_addr);
        cu->csdrf_wr_from(csdrf_wr_from);
        // Shift and Add control
        cu->sa_en(sa_en);
        cu->sa_shift(sa_shift);
        cu->sa_size(sa_size);
        cu->sa_adder_en(sa_adder_en);
        cu->sa_neg_op1(sa_neg_op1);
        cu->sa_neg_op2(sa_neg_op2);
        cu->sa_op1_from(sa_op1_from);
        cu->sa_op2_from(sa_op2_from);
        // Pack and Mask control
        cu->pm_en(pm_en);
        cu->pm_repack(pm_repack);
        cu->pm_in_start(pm_in_start);
        cu->pm_op_sel(pm_op_sel);
        cu->pm_shift(pm_shift);
        cu->pm_op1_from(pm_op1_from);
        cu->pm_op2_from(pm_op2_from);
#if DUAL_BANK_INTERFACE
        // BANKS Control
        cu->even_out_en(even_out_en);
        cu->odd_out_en(odd_out_en);
#else
        cu->dram_out_en(dram_out_en);
        cu->dram_from(dram_from);
#endif

        sa_stage = new shift_and_add("Shift&Add");
        sa_stage->clk(clk);
        sa_stage->rst(rst);
        sa_stage->compute_en(sa_en);
        sa_stage->shift(sa_shift);
        sa_stage->size(sa_size);
        sa_stage->adder_en(sa_adder_en);
        sa_stage->neg_op1(sa_neg_op1);
        sa_stage->neg_op2(sa_neg_op2);
        for (i=0; i<WORD_64B; i++) {
            sa_stage->op1[i](sa_op1[i]);
            sa_stage->op2[i](sa_op2[i]);
            sa_stage->output[i](sa_out[i]);
        }

        pm_stage = new pack_and_mask("Pack&Mask");
        pm_stage->clk(clk);
        pm_stage->rst(rst);
        pm_stage->compute_en(pm_en);
        for (i=0; i<WORD_64B; i++) {
            pm_stage->w1[i](pm_w1[i]);
            pm_stage->w2[i](pm_w2[i]);
        }
        pm_stage->repack(pm_repack);
        pm_stage->in_start(pm_in_start);
        for (i=0; i<MASK_64B; i++)
            pm_stage->mask_in[i](mrf_out[i]);
        pm_stage->op_sel(pm_op_sel);
        pm_stage->shift(pm_shift);
        for (i=0; i<WORD_64B; i++)
            pm_stage->output[i](pm_out[i]);

        ts = new tile_shuffler("Tile_shuffler");
        ts->clk(clk);
        ts->rst(rst);
        ts->input_en(ts_in_en);
        ts->output_en(ts_out_en);
        ts->out_start(ts_out_start);
        ts->mode(ts_mode);
        ts->vww_in(ts_in);
        ts->vww_out(ts_out);

        ib_macro = new rf_twoport<uint64_t, IB_ENTRIES>("Macroinstruction_buffer");
        ib_macro->clk(clk);
        ib_macro->rst(rst);
        ib_macro->rd_addr(PC);
        ib_macro->rd_port(macroinstr);
        ib_macro->wr_en(ib_wr_en);
        ib_macro->wr_addr(ib_wr_addr);
        ib_macro->wr_port(ib_in);

        for (i=0; i<MASK_64B; i++) {
            srf[i] = new rf_twoport<uint64_t, SRF_ENTRIES>(sc_gen_unique_name("SRF"));
            srf[i]->clk(clk);
            srf[i]->rst(rst);
            srf[i]->rd_addr(srf_rd_addr);
            srf[i]->rd_port(srf_out[i]);
            srf[i]->wr_en(srf_wr_en);
            srf[i]->wr_addr(srf_wr_addr);
            srf[i]->wr_port(srf_in[i]);
        }

        for (i=0; i<MASK_64B; i++) {
            mrf[i] = new rf_twoport<uint64_t, MASK_ENTRIES>(sc_gen_unique_name("MRF"));
            mrf[i]->clk(clk);
            mrf[i]->rst(rst);
            mrf[i]->rd_addr(mrf_rd_addr);
            mrf[i]->rd_port(mrf_out[i]);
            mrf[i]->wr_en(mrf_wr_en);
            mrf[i]->wr_addr(mrf_wr_addr);
            mrf[i]->wr_port(mrf_in[i]);
        }

        for (i=0; i<CSD_64B; i++) {
            csdrf[i] = new rf_twoport<uint64_t, CSD_ENTRIES>(sc_gen_unique_name("CSD_RF"));
            csdrf[i]->clk(clk);
            csdrf[i]->rst(rst);
            csdrf[i]->rd_addr(csdrf_rd_addr);
            csdrf[i]->rd_port(csdrf_out[i]);
            csdrf[i]->wr_en(csdrf_wr_en);
            csdrf[i]->wr_addr(csdrf_wr_addr);
            csdrf[i]->wr_port(csdrf_in[i]);
        }

        for (i=0; i<REG_NUM; i++) {
            R[i] = new reg<WORD_64B>(sc_gen_unique_name("R"));
            R[i]->clk(clk);
            R[i]->rst(rst);
            R[i]->en(reg_en[i]);
            for (j=0; j<WORD_64B; j++) {
                R[i]->input[j](R_in[i][j]);
                R[i]->output[j](R_out[i][j]);
            }
        }

#if VWR_DRAM_CLK > 1
        for (i=0; i<VWR_NUM; i++) {
            vwreg[i] = new vwr_multicycle<VWR_BITS, WORD_BITS, DRAM_BITS>(sc_gen_unique_name("VWR"));
            vwreg[i]->clk(clk);
            vwreg[i]->rst(rst);
            vwreg[i]->enable(vwr_enable[i]);
            vwreg[i]->wr_nrd(vwr_wr_nrd[i]);
            vwreg[i]->dram_idx(vwr_dram_idx);
            vwreg[i]->dram_d_nm(vwr_dram_d_nm[i]);
            vwreg[i]->idx(vwr_idx[i]);
            vwreg[i]->d_nm(vwr_d_nm[i]);
            vwreg[i]->lport(vwr_inout[i]);
            vwreg[i]->sport(vwr_muxed[i]);
            vwreg[i]->dport(vwr_dram_if[i]);
        }
#else
        for (i=0; i<VWR_NUM; i++) {
            vwreg[i] = new vwr<VWR_BITS, WORD_BITS, WORDS_PER_VWR>(sc_gen_unique_name("VWR"));
            vwreg[i]->clk(clk);
            vwreg[i]->rst(rst);
            vwreg[i]->enable(vwr_enable[i]);
            vwreg[i]->wr_nrd(vwr_wr_nrd[i]);
            vwreg[i]->idx(vwr_idx[i]);
            vwreg[i]->mask_en(vwr_mask_en[i]);
            vwreg[i]->mask(vwr_mask[i]);
            vwreg[i]->d_nm(vwr_d_nm[i]);
            vwreg[i]->lport_in(vwr_in[i]);
            vwreg[i]->lport_out(vwr_out[i]);
            vwreg[i]->sport_in(vwr_muxed_in[i]);
            vwreg[i]->sport_out(vwr_muxed_out[i]);
        }
#endif

        SC_METHOD(comb_method);
        sensitive << ts_shf_from << ts_out;
        sensitive << srf_wr_from << mrf_wr_from << csdrf_wr_from;
        sensitive << sa_op1_from << sa_op2_from << pm_op1_from << pm_op2_from;
        for (int i=0; i<MASK_64B; i++)
            sensitive << srf_out[i] << mrf_out[i];
        for (int i=0; i<WORD_64B; i++)
            sensitive << sa_out[i] << pm_out[i] << cu_data_out;
        for (int i=0; i<REG_NUM; i++) {
            sensitive << reg_from[i];
            for (int j=0; j<WORD_64B; j++)
                sensitive << R_out[i][j];
        }
        for (int i=0; i<VWR_NUM; i++)
            sensitive << vwr_wr_nrd[i] << vwr_from[i] << vwr_out[i] << vwr_muxed_out[i];
#if DUAL_BANK_INTERFACE
        sensitive << even_in << odd_in << even_out_en << odd_out_en;
#else
        sensitive << dram_in << dram_from << dram_out_en;
#endif

        SC_METHOD(adaptation_method);
        for (int i=0; i<VWR_64B; i++)
            sensitive << cu_data_out;
    }

    void comb_method();
    void adaptation_method();   // Adapts C types to SystemC types

#endif  // __SYNTHESIS__

};

#endif /* SRC_SOFTSIMD_PU_H_ */

#endif
