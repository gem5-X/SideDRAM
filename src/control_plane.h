/*
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 *
 * Description of the control plane for area and energy assessment.
 *
 */

#include "cnm_base.h"

#if EN_MODEL == 0

#ifndef SRC_CONTROL_PLANE_H_
#define SRC_CONTROL_PLANE_H_

#include "control_unit.h"
#include "rf_twoport.h"

class control_plane: public sc_module {
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
    sc_out<uint64_t>            data_out;   // Data to the Registers
    
    // Tile shuffler control
    sc_out<bool>    ts_in_en;       // Enable input to tile shuffle
    sc_out<bool>    ts_out_en;      // Enable output from tile shuffle
    sc_out<uint>    ts_out_start;   // Start location for the words shuffled out
    sc_out<TS_MODE> ts_out_mode;    // Shuffling mode
    sc_out<MUX>     ts_shf_from;    // Index the MUX for input data

    // VWRs control
    sc_out<bool>        vwr_enable[VWR_NUM];    // Enable signals for the VWRs
    sc_out<bool>        vwr_wr_nrd[VWR_NUM];    // Read/write signals for the VWRs
#if VWR_DRAM_CLK > 1
    sc_out<bool>        vwr_dram_d_nm[VWR_NUM]; // Multiplex/demultiplex signals for the VWRs-DRAM interface
    sc_out<uint>        vwr_dram_idx;           // Index of the VWRs-DRAM mux/demux
#endif
    sc_out<bool>        vwr_d_nm[VWR_NUM];      // Multiplex/demultiplex signals for the VWRs
    sc_out<bool>        vwr_mask_en[VWR_NUM];   // Enable VWR masked access
    sc_out<uint64_t>    vwr_mask[VWR_NUM];      // Word mask for VWR accesses to and from DRAM
    sc_out<uint>        vwr_idx[VWR_NUM];       // Word indeces of the VWR accesses
    sc_out<MUX>         vwr_from[VWR_NUM];      // Index the MUX for input data

    // Registers control
    sc_out<bool>    reg_en[REG_NUM];    // Write enable for the registers in the datapath
    sc_out<MUX>     reg_from[REG_NUM];  // Index the MUX for input data

    // SRF control
    sc_out<uint>    srf_rd_addr;    // Index read
    sc_out<bool>    srf_wr_en;      // Enable writing
    sc_out<uint>    srf_wr_addr;    // Index the address to be written
    sc_out<MUX>     srf_wr_from;    // Index the MUX for input data

    // Mask RF control
    sc_out<uint>    mrf_rd_addr;    // Index read
    sc_out<bool>    mrf_wr_en;      // Enable writing
    sc_out<uint>    mrf_wr_addr;    // Index the address to be written
    sc_out<MUX>     mrf_wr_from;    // Index the MUX for input data

    // Shift and Add control
    sc_out<bool>    sa_en;          // Enables shift & add stage
    sc_out<uint>    sa_shift;       // Specifies shift
    sc_out<SWSIZE>  sa_size;        // Specifies subword size
    sc_out<bool>    sa_adder_en;    // Enables adder/subtractor
    sc_out<bool>    sa_neg_op1;     // Signals if subtracting op1
    sc_out<bool>    sa_neg_op2;     // Signals if subtracting op2
    sc_out<MUX>     sa_op1_from;    // Index MUX for input data 1
    sc_out<MUX>     sa_op2_from;    // Index MUX for input data 2

    // Pack and Mask control
    sc_out<bool>        pm_en;          // Enables pack & mask stage
    sc_out<SWREPACK>    pm_repack;      // Control for the repacker
    sc_out<uint>        pm_in_start;    // Position of subword in w1+w2 where packing to output starts
    sc_out<MASKOP>      pm_op_sel;      // Selection of mask operation
    sc_out<uint>        pm_shift;       // Positions to shift right
    sc_out<MUX>         pm_op1_from;    // Index MUX for input data 1
    sc_out<MUX>         pm_op2_from;    // Index MUX for input data 2

#if DUAL_BANK_INTERFACE
    // BANKS Control
    sc_out<bool>    even_out_en;   // Enables the even bank tri-state buffer
    sc_out<bool>    odd_out_en;    // Enables the odd bank tri-state buffer
#else
    sc_out<bool>    dram_out_en;    // Enables the DRAM tri-state buffer
    sc_out<MUX>     dram_from;      // Index the MUX for input data
#endif

    // ** INTERNAL SIGNALS AND VARIABLES **
    // Basic control
    sc_signal<uint64_t> macroinstr;
    sc_signal<uint>     PC;
    sc_signal<uint64_t> cu_data_out;
    // Instruction buffer
    sc_signal<bool>     ib_wr_en;
    sc_signal<uint>     ib_wr_addr;
    sc_signal<uint64_t> ib_in;
    // CSD RF
    sc_signal<uint>     csdrf_rd_addr, csdrf_wr_addr;
    sc_signal<bool>     csdrf_wr_en;
    sc_signal<MUX>      csdrf_wr_from;
    sc_signal<uint64_t> csdrf_in[WORD_64B], csdrf_out[WORD_64B];

    // Internal modules
    control_unit *cu;
    rf_twoport<uint64_t,IB_ENTRIES> *ib_macro;
    rf_twoport<uint64_t, CSD_ENTRIES> *csdrf[CSD_64B];

    SC_HAS_PROCESS(control_plane);
    control_plane(sc_module_name name) : sc_module(name) {

        int i;

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
        cu->ts_out_mode(ts_out_mode);
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

        ib_macro = new rf_twoport<uint64_t, IB_ENTRIES>("Macroinstruction_buffer");
        ib_macro->clk(clk);
        ib_macro->rst(rst);
        ib_macro->rd_addr(PC);
        ib_macro->rd_port(macroinstr);
        ib_macro->wr_en(ib_wr_en);
        ib_macro->wr_addr(ib_wr_addr);
        ib_macro->wr_port(ib_in);

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

        SC_METHOD(comb_method);
        sensitive << cu_data_out;

        SC_METHOD(adaptation_method);
        sensitive << cu_data_out;
    }

    void comb_method();
    void adaptation_method();   // Adapts C types to SystemC types

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
    sc_out<uint64_t>            data_out;   // Data to the Registers

    // Tile shuffler control
    sc_out<bool>    ts_in_en;       // Enable input to tile shuffle
    sc_out<bool>    ts_out_en;      // Enable output from tile shuffle
    sc_out<uint>    ts_out_start;   // Start location for the words shuffled out
    sc_out<TS_MODE> ts_out_mode;    // Shuffling mode
    sc_out<MUX>     ts_shf_from;    // Index the MUX for input data

    // VWRs control
    sc_out<bool>        vwr_enable[VWR_NUM];    // Enable signals for the VWRs
    sc_out<bool>        vwr_wr_nrd[VWR_NUM];    // Read/write signals for the VWRs
#if VWR_DRAM_CLK > 1
    sc_out<bool>        vwr_dram_d_nm[VWR_NUM]; // Multiplex/demultiplex signals for the VWRs-DRAM interface
    sc_out<uint>        vwr_dram_idx;           // Index of the VWRs-DRAM mux/demux
#endif
    sc_out<bool>        vwr_d_nm[VWR_NUM];      // Multiplex/demultiplex signals for the VWRs
    sc_out<bool>        vwr_mask_en[VWR_NUM];   // Enable VWR masked access
    sc_out<uint64_t>    vwr_mask[VWR_NUM];      // Word mask for VWR accesses to and from DRAM
    sc_out<uint>        vwr_idx[VWR_NUM];       // Word indeces of the VWR accesses
    sc_out<MUX>         vwr_from[VWR_NUM];      // Index the MUX for input data

    // Registers control
    sc_out<bool>    reg_en[REG_NUM];    // Write enable for the registers in the datapath
    sc_out<MUX>     reg_from[REG_NUM];  // Index the MUX for input data

    // SRF control
    sc_out<uint>    srf_rd_addr;    // Index read
    sc_out<bool>    srf_wr_en;      // Enable writing
    sc_out<uint>    srf_wr_addr;    // Index the address to be written
    sc_out<MUX>     srf_wr_from;    // Index the MUX for input data

    // Mask RF control
    sc_out<uint>    mrf_rd_addr;    // Index read
    sc_out<bool>    mrf_wr_en;      // Enable writing
    sc_out<uint>    mrf_wr_addr;    // Index the address to be written
    sc_out<MUX>     mrf_wr_from;    // Index the MUX for input data

    // Shift and Add control
    sc_out<bool>    sa_en;          // Enables shift & add stage
    sc_out<uint>    sa_shift;       // Specifies shift
    sc_out<SWSIZE>  sa_size;        // Specifies subword size
    sc_out<bool>    sa_adder_en;    // Enables adder/subtractor
    sc_out<bool>    sa_neg_op1;     // Signals if subtracting op1
    sc_out<bool>    sa_neg_op2;     // Signals if subtracting op2
    sc_out<MUX>     sa_op1_from;    // Index MUX for input data 1
    sc_out<MUX>     sa_op2_from;    // Index MUX for input data 2

    // Pack and Mask control
    sc_out<bool>        pm_en;          // Enables pack & mask stage
    sc_out<SWREPACK>    pm_repack;      // Control for the repacker
    sc_out<uint>        pm_in_start;    // Position of subword in w1+w2 where packing to output starts
    sc_out<MASKOP>      pm_op_sel;      // Selection of mask operation
    sc_out<uint>        pm_shift;       // Positions to shift right
    sc_out<MUX>         pm_op1_from;    // Index MUX for input data 1
    sc_out<MUX>         pm_op2_from;    // Index MUX for input data 2

#if DUAL_BANK_INTERFACE
    // BANKS Control
    sc_out<bool>    even_out_en;   // Enables the even bank tri-state buffer
    sc_out<bool>    odd_out_en;    // Enables the odd bank tri-state buffer
#else
    sc_out<bool>    dram_out_en;    // Enables the DRAM tri-state buffer
    sc_out<MUX>     dram_from;      // Index the MUX for input data
#endif

    // ** INTERNAL SIGNALS AND VARIABLES **
    // Basic control
    sc_signal<uint64_t> macroinstr;
    sc_signal<uint>     PC;
    sc_signal<uint64_t> cu_data_out;
    // Instruction buffer
    sc_signal<bool>     ib_wr_en;
    sc_signal<uint>     ib_wr_addr;
    sc_signal<uint64_t> ib_in;
    // CSD RF
    sc_signal<uint>     csdrf_rd_addr, csdrf_wr_addr;
    sc_signal<bool>     csdrf_wr_en;
    sc_signal<MUX>      csdrf_wr_from;
    sc_signal<uint64_t> csdrf_in[WORD_64B], csdrf_out[WORD_64B];

    // Internal modules
    control_unit *cu;
    rf_twoport<uint64_t, IB_ENTRIES> *ib_macro;
    rf_twoport<uint64_t, CSD_ENTRIES> *csdrf[CSD_64B];

    SC_HAS_PROCESS(control_plane);
    control_plane(sc_module_name name) : sc_module(name) {
        int i;

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
        cu->ts_out_mode(ts_out_mode);
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

        ib_macro = new rf_twoport<uint64_t, IB_ENTRIES>("Macroinstruction_buffer");
        ib_macro->clk(clk);
        ib_macro->rst(rst);
        ib_macro->rd_addr(PC);
        ib_macro->rd_port(macroinstr);
        ib_macro->wr_en(ib_wr_en);
        ib_macro->wr_addr(ib_wr_addr);
        ib_macro->wr_port(ib_in);

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

        SC_METHOD(comb_method);
        sensitive << cu_data_out;

        SC_METHOD(adaptation_method);
        sensitive << cu_data_out;
    }

    void comb_method();
    void adaptation_method();   // Adapts C types to SystemC types

#endif  // __SYNTHESIS__

};

#endif /* SRC_CONTROL_PLANE_H_ */
#endif  // EN_MODEL == 0
