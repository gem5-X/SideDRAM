/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the SoftSIMD Processing Unit, testing the RTL of the control unit.
 *
 */

#include "cnm_base.h"

#if MIXED_SIM

#ifndef SRC_SOFTSIMD_PU_H_
#define SRC_SOFTSIMD_PU_H_

#include "cu_wrapped.h"
#include "pack_and_mask.h"
#include "register.h"
#include "rf_twoport.h"
#include "shift_and_add.h"
#include "tile_shuffler.h"
#include "tristate_buffer.h"
#include "vwr.h"

bool logic_to_bool (sc_logic in);
sc_logic bool_to_logic (bool in);

class softsimd_pu: public sc_module {
public:

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
#ifdef __SYNTHESIS__
#if DUAL_BANK_INTERFACE
    sc_in<sc_biguint<DRAM_BITS> >   even_in;    // Direct data in/out to the even bank
    sc_in<sc_biguint<DRAM_BITS> >   odd_in;     // Direct data in/out to the even bank
    sc_out<sc_biguint<DRAM_BITS> >  even_out;   // Direct data in/out to the even bank
    sc_out<sc_biguint<DRAM_BITS> >  odd_out;    // Direct data in/out to the even bank
#else
    sc_in<sc_biguint<DRAM_BITS> >   dram_in;    // Direct data in/out to the even bank
    sc_out<sc_biguint<DRAM_BITS> >  dram_out;   // Direct data in/out to the even bank
#endif
#else
#if DUAL_BANK_INTERFACE
    sc_inout_rv<DRAM_BITS>      even_bus;   // Direct data in/out to the even bank
    sc_inout_rv<DRAM_BITS>      odd_bus;    // Direct data in/out to the odd bank
#else
    sc_inout_rv<DRAM_BITS>      dram_bus;   // Direct data in/out to the DRAM
#endif
#endif

    // ** INTERNAL SIGNALS AND VARIABLES **
    // Basic control
    sc_signal<uint32_t> macroinstr;
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
    sc_signal<uint32_t> ib_in;
    // Tile shuffler
    sc_signal<bool>         ts_in_en, ts_out_en;
    sc_signal<uint>         ts_out_start;
    sc_signal<uint>         ts_mode;
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
    sc_signal<bool>     sa_en, sa_adder_en, sa_s_na;
    sc_signal<uint>     sa_shift;
    sc_signal<SWSIZE>   sa_size;
    sc_signal<MUX>      sa_op1_from, sa_op2_from;
    sc_signal<uint64_t> sa_op1[WORD_64B], sa_op2[WORD_64B], sa_out[WORD_64B];
    // Pack & Mask stage
    sc_signal<bool>     pm_en;
    sc_signal<SWSIZE>   pm_in_size, pm_out_size;
    sc_signal<uint>     pm_in_start, pm_shift;
    sc_signal<MASKOP>   pm_op_sel;
    sc_signal<MUX>      pm_op1_from, pm_op2_from;
    sc_signal<uint64_t> pm_w1[WORD_64B], pm_w2[WORD_64B], pm_out[WORD_64B];

    // Signals for mixed signal simulation with control unit
     sc_signal<sc_logic>            clk_mixed, rst_mixed;
     sc_signal<sc_logic>            RD_mixed, WR_mixed, ACT_mixed, AB_mode_mixed, pim_mode_mixed;
     sc_signal<sc_lv<BANK_BITS> >   bank_addr_mixed;
     sc_signal<sc_lv<ROW_BITS> >    row_addr_mixed;
     sc_signal<sc_lv<COL_BITS> >    col_addr_mixed;
     sc_signal<sc_lv<DQ_BITS> >     DQ_mixed;
     sc_signal<sc_lv<32> >          macroinstr_mixed;
     sc_signal<sc_lv<64> >          csdrf_out_mixed[CSD_64B];
     sc_signal<sc_lv<32> >          PC_out_mixed;
     sc_signal<sc_lv<64> >          cu_data_out_mixed;
     sc_signal<sc_logic>            ib_wr_en_mixed;
     sc_signal<sc_lv<32> >          ib_wr_addr_mixed;
     sc_signal<sc_logic>            ts_in_en_mixed;
     sc_signal<sc_logic>            ts_out_en_mixed;
     sc_signal<sc_lv<32> >          ts_out_start_mixed;
     sc_signal<sc_lv<32> >          ts_mode_mixed;
     sc_signal<sc_lv<32> >          ts_shf_from_mixed;
     sc_signal<sc_logic>            vwr_enable_mixed[VWR_NUM], vwr_wr_nrd_mixed[VWR_NUM];
     sc_signal<sc_logic>            vwr_d_nm_mixed[VWR_NUM], vwr_mask_en_mixed[VWR_NUM];
     sc_signal<sc_lv<64> >          vwr_mask_mixed[VWR_NUM];
     sc_signal<sc_lv<32> >          vwr_idx_mixed[VWR_NUM], vwr_from_mixed[VWR_NUM];
#if VWR_DRAM_CLK > 1
     sc_signal<sc_logic>            vwr_dram_d_nm_mixed[VWR_NUM];
     sc_signal<sc_lv<32> >          vwr_dram_idx_mixed;
#endif
     sc_signal<sc_logic>            reg_en_mixed[REG_NUM];
     sc_signal<sc_lv<32> >          reg_from_mixed[REG_NUM];
     sc_signal<sc_lv<32> >          srf_rd_addr_mixed, mrf_rd_addr_mixed, csdrf_rd_addr_mixed;
     sc_signal<sc_logic>            srf_wr_en_mixed, mrf_wr_en_mixed, csdrf_wr_en_mixed;
     sc_signal<sc_lv<32> >          srf_wr_addr_mixed, mrf_wr_addr_mixed, csdrf_wr_addr_mixed;
     sc_signal<sc_lv<32> >          srf_wr_from_mixed, mrf_wr_from_mixed, csdrf_wr_from_mixed;
     sc_signal<sc_logic>            sa_en_mixed, sa_adder_en_mixed, sa_s_na_mixed;
     sc_signal<sc_lv<32> >          sa_shift_mixed, sa_size_mixed;
     sc_signal<sc_lv<32> >          sa_op1_from_mixed, sa_op2_from_mixed;
     sc_signal<sc_logic>            pm_en_mixed;
     sc_signal<sc_lv<32> >          pm_in_size_mixed, pm_out_size_mixed, pm_in_start_mixed;
     sc_signal<sc_lv<32> >          pm_op_sel_mixed, pm_shift_mixed;
     sc_signal<sc_lv<32> >          pm_op1_from_mixed, pm_op2_from_mixed;
#if DUAL_BANK_INTERFACE
     sc_signal<sc_logic>            even_out_en_mixed, odd_out_en_mixed;
#else
     sc_signal<sc_logic>            dram_out_en_mixed;
     sc_signal<sc_lv<32> >          dram_from_mixed;
#endif

    // Internal modules
    control_unit *cu;
    shift_and_add *sa_stage;
    pack_and_mask *pm_stage;
    tile_shuffler *ts;
    rf_twoport<uint32_t,IB_ENTRIES> *ib_macro;
    rf_twoport<uint64_t, CSD_ENTRIES> *csdrf[CSD_64B];
    rf_twoport<uint64_t, SRF_ENTRIES> *srf[MASK_64B];
    rf_twoport<uint64_t, MASK_ENTRIES> *mrf[MASK_64B];
    reg<WORD_64B> *R[REG_NUM];
#if VWR_DRAM_CLK > 1
    vwr_multicycle<VWR_BITS, WORD_BITS, DRAM_BITS> *vwreg[VWR_NUM];
#else
    vwr<VWR_BITS, WORD_BITS> *vwreg[VWR_NUM];
#endif
#ifndef __SYNTHESIS__
#if DUAL_BANK_INTERFACE
    tristate_buffer<DRAM_BITS> *even_buf, *odd_buf;
#else
    tristate_buffer<DRAM_BITS> *dram_buf;
#endif
#endif // __SYNTHESIS__

    SC_HAS_PROCESS(softsimd_pu);
    softsimd_pu(sc_module_name name) : sc_module(name) {
        int i, j;

        cu = new control_unit("control_unit", "control_unit");
        cu->clk(clk_mixed);
        cu->rst(rst_mixed);
        cu->RD(RD_mixed);
        cu->WR(WR_mixed);
        cu->ACT(ACT_mixed);
//      cu->RSTB(RSTB);
        cu->AB_mode(AB_mode_mixed);
        cu->pim_mode(pim_mode_mixed);
        cu->bank_addr(bank_addr_mixed);
        cu->row_addr(row_addr_mixed);
        cu->col_addr(col_addr_mixed);
        cu->DQ(DQ_mixed);
        cu->macroinstr(macroinstr_mixed);
        cu->csd_mult_0(csdrf_out_mixed[0]);
        cu->pc_out(PC_out_mixed);
        cu->data_out(cu_data_out_mixed);
        // Instruction buffer control
        cu->ib_wr_en(ib_wr_en_mixed);
        cu->ib_wr_addr(ib_wr_addr_mixed);
        // Tile shuffler control
        cu->ts_in_en(ts_in_en_mixed);
        cu->ts_out_en(ts_out_en_mixed);
        cu->ts_out_start(ts_out_start_mixed);
        cu->ts_out_mode(ts_mode_mixed);
        cu->ts_shf_from(ts_shf_from_mixed);
        // VWRs control
        cu->vwr_enable_0(vwr_enable_mixed[0]);
        cu->vwr_enable_1(vwr_enable_mixed[1]);
        cu->vwr_enable_2(vwr_enable_mixed[2]);
        cu->vwr_enable_3(vwr_enable_mixed[3]);
        cu->vwr_wr_nrd_0(vwr_wr_nrd_mixed[0]);
        cu->vwr_wr_nrd_1(vwr_wr_nrd_mixed[1]);
        cu->vwr_wr_nrd_2(vwr_wr_nrd_mixed[2]);
        cu->vwr_wr_nrd_3(vwr_wr_nrd_mixed[3]);
        cu->vwr_d_nm_0(vwr_d_nm_mixed[0]);
        cu->vwr_d_nm_1(vwr_d_nm_mixed[1]);
        cu->vwr_d_nm_2(vwr_d_nm_mixed[2]);
        cu->vwr_d_nm_3(vwr_d_nm_mixed[3]);
#if VWR_DRAM_CLK > 1
        cu->vwr_dram_d_nm_0(vwr_dram_d_nm_mixed[0]);
        cu->vwr_dram_d_nm_1(vwr_dram_d_nm_mixed[1]);
        cu->vwr_dram_d_nm_2(vwr_dram_d_nm_mixed[2]);
        cu->vwr_dram_d_nm_3(vwr_dram_d_nm_mixed[3]);
#endif
        cu->vwr_mask_en_0(vwr_mask_en_mixed[0]);
        cu->vwr_mask_en_1(vwr_mask_en_mixed[1]);
        cu->vwr_mask_en_2(vwr_mask_en_mixed[2]);
        cu->vwr_mask_en_3(vwr_mask_en_mixed[3]);
        cu->vwr_mask_0(vwr_mask_mixed[0]);
        cu->vwr_mask_1(vwr_mask_mixed[1]);
        cu->vwr_mask_2(vwr_mask_mixed[2]);
        cu->vwr_mask_3(vwr_mask_mixed[3]);
        cu->vwr_idx_0(vwr_idx_mixed[0]);
        cu->vwr_idx_1(vwr_idx_mixed[1]);
        cu->vwr_idx_2(vwr_idx_mixed[2]);
        cu->vwr_idx_3(vwr_idx_mixed[3]);
        cu->vwr_from_0(vwr_from_mixed[0]);
        cu->vwr_from_1(vwr_from_mixed[1]);
        cu->vwr_from_2(vwr_from_mixed[2]);
        cu->vwr_from_3(vwr_from_mixed[3]);
#if VWR_DRAM_CLK > 1
        cu->vwr_dram_idx(vwr_dram_idx_mixed);
#endif
        // Registers control
        cu->reg_en_0(reg_en_mixed[0]);
        cu->reg_en_1(reg_en_mixed[1]);
        cu->reg_en_2(reg_en_mixed[2]);
        cu->reg_en_3(reg_en_mixed[3]);
        cu->reg_from_0(reg_from_mixed[0]);
        cu->reg_from_1(reg_from_mixed[1]);
        cu->reg_from_2(reg_from_mixed[2]);
        cu->reg_from_3(reg_from_mixed[3]);
        // SRF control
        cu->srf_rd_addr(srf_rd_addr_mixed);
        cu->srf_wr_en(srf_wr_en_mixed);
        cu->srf_wr_addr(srf_wr_addr_mixed);
        cu->srf_wr_from(srf_wr_from_mixed);
        // Mask RF control
        cu->mrf_rd_addr(mrf_rd_addr_mixed);
        cu->mrf_wr_en(mrf_wr_en_mixed);
        cu->mrf_wr_addr(mrf_wr_addr_mixed);
        cu->mrf_wr_from(mrf_wr_from_mixed);
        // CSD RF control
        cu->csdrf_rd_addr(csdrf_rd_addr_mixed);
        cu->csdrf_wr_en(csdrf_wr_en_mixed);
        cu->csdrf_wr_addr(csdrf_wr_addr_mixed);
        cu->csdrf_wr_from(csdrf_wr_from_mixed);
        // Shift and Add control
        cu->sa_en(sa_en_mixed);
        cu->sa_shift(sa_shift_mixed);
        cu->sa_size(sa_size_mixed);
        cu->sa_adder_en(sa_adder_en_mixed);
        cu->sa_s_na(sa_s_na_mixed);
        cu->sa_op1_from(sa_op1_from_mixed);
        cu->sa_op2_from(sa_op2_from_mixed);
        // Pack and Mask control
        cu->pm_en(pm_en_mixed);
        cu->pm_in_size(pm_in_size_mixed);
        cu->pm_out_size(pm_out_size_mixed);
        cu->pm_in_start(pm_in_start_mixed);
        cu->pm_op_sel(pm_op_sel_mixed);
        cu->pm_shift(pm_shift_mixed);
        cu->pm_op1_from(pm_op1_from_mixed);
        cu->pm_op2_from(pm_op2_from_mixed);
#if DUAL_BANK_INTERFACE
        // BANKS Control
        cu->even_out_en(even_out_en_mixed);
        cu->odd_out_en(odd_out_en_mixed);
#else
        cu->dram_out_en(dram_out_en_mixed);
        cu->dram_from(dram_from_mixed);
#endif

        sa_stage = new shift_and_add("Shift&Add");
        sa_stage->clk(clk);
        sa_stage->rst(rst);
        sa_stage->compute_en(sa_en);
        sa_stage->shift(sa_shift);
        sa_stage->size(sa_size);
        sa_stage->adder_en(sa_adder_en);
        sa_stage->s_na(sa_s_na);
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
        pm_stage->in_size(pm_in_size);
        pm_stage->out_size(pm_out_size);
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

        ib_macro = new rf_twoport<uint32_t, IB_ENTRIES>("Macroinstruction_buffer");
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
            vwreg[i] = new vwr<VWR_BITS, WORD_BITS>(sc_gen_unique_name("VWR"));
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

#ifndef __SYNTHESIS__
#if DUAL_BANK_INTERFACE
        even_buf = new tristate_buffer<DRAM_BITS>("Even_tristate_buffer");
#if VWR_DRAM_CLK > 1
        even_buf->input(vwr_dram_if[0]);
#else
        even_buf->input(vwr_inout[0]);
#endif
        even_buf->enable(even_out_en);
        even_buf->output(even_bus);

        odd_buf = new tristate_buffer<DRAM_BITS>("Odd_tristate_buffer");
#if VWR_DRAM_CLK > 1
        odd_buf->input(vwr_dram_if[1]);
#else
        odd_buf->input(vwr_inout[1]);
#endif
        odd_buf->enable(odd_out_en);
        odd_buf->output(odd_bus);
#else
        dram_buf = new tristate_buffer<DRAM_BITS>("DRAM_tristate_buffer");
        dram_buf->input(to_dram);
        dram_buf->enable(dram_out_en);
        dram_buf->output(dram_bus);
#endif // DUAL_BANK_INTERFACE
#endif // __SYNTHESIS__

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
#ifdef __SYNTHESIS
#if DUAL_BANK_INTERFACE
        sensitive << even_in << odd_in;
#else
        sensitive << dram_in << dram_from;
#endif
        sensitive << to_dram;
#else
#if DUAL_BANK_INTERFACE
        sensitive << even_bus << odd_bus;
#else
        sensitive << dram_bus << dram_from;
#endif
#endif

        SC_METHOD(adaptation_method);
        for (int i=0; i<VWR_64B; i++)
            sensitive << cu_data_out;

        SC_METHOD(mixed_method);
        sensitive << clk << rst << RD << WR << ACT << AB_mode << pim_mode;
        sensitive << bank_addr << row_addr << col_addr << DQ << macroinstr;
        for (int i=0; i<CSD_64B; i++)
            sensitive << csdrf_out[i];
        sensitive << PC_out_mixed << cu_data_out_mixed;
        sensitive << ib_wr_en_mixed << ib_wr_addr_mixed;
        sensitive << ts_in_en_mixed << ts_out_en_mixed << ts_out_start_mixed << ts_mode_mixed << ts_shf_from_mixed;
        for (int i=0; i<VWR_NUM; i++) {
            sensitive << vwr_enable_mixed[i] << vwr_wr_nrd_mixed[i];
            sensitive << vwr_d_nm_mixed[i] << vwr_mask_en_mixed[i];
            sensitive << vwr_mask_mixed[i] << vwr_idx_mixed[i] << vwr_from_mixed[i];
#if VWR_DRAM_CLK > 1
            sensitive << vwr_dram_d_nm_mixed[i];
#endif
        }
#if VWR_DRAM_CLK > 1
        sensitive << vwr_dram_idx_mixed;
#endif
        for (int i=0; i<REG_NUM; i++) {
            sensitive << reg_en_mixed[i] << reg_from_mixed[i];
        }
        sensitive << srf_rd_addr_mixed << mrf_rd_addr_mixed << csdrf_rd_addr_mixed;
        sensitive << srf_wr_en_mixed << mrf_wr_en_mixed << csdrf_wr_en_mixed;
        sensitive << srf_wr_addr_mixed << mrf_wr_addr_mixed << csdrf_wr_addr_mixed;
        sensitive << srf_wr_from_mixed << mrf_wr_from_mixed << csdrf_wr_from_mixed;
        sensitive << sa_en_mixed << sa_adder_en_mixed << sa_s_na_mixed;
        sensitive << sa_shift_mixed << sa_size_mixed;
        sensitive << sa_op1_from_mixed << sa_op2_from_mixed;
        sensitive << pm_en_mixed;
        sensitive << pm_in_size_mixed << pm_out_size_mixed << pm_in_start_mixed;
        sensitive << pm_op_sel_mixed << pm_shift_mixed;
        sensitive << pm_op1_from_mixed << pm_op2_from_mixed;
#if DUAL_BANK_INTERFACE
        sensitive << even_out_en_mixed << odd_out_en_mixed;
#else
        sensitive << dram_out_en_mixed << dram_from_mixed;
#endif

    }

    void comb_method();
    void adaptation_method();   // Adapts C types to SystemC types
    void mixed_method();
};

#endif /* SRC_SOFTSIMD_PU_H_ */

#endif
