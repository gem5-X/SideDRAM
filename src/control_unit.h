/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the Control Unit for SoftSIMD. // NOTE when synthesizing for SoftSIMD, deactivate flattening here (only for submodules)
 *
 */

#ifndef SRC_CONTROL_UNIT_H_
#define SRC_CONTROL_UNIT_H_

#include "cnm_base.h"
#include "itt_rom.h"
#include "dlb_rom.h"
#include "mult_sequencer.h"
#include "instruction_decoder_mov.h"
#include "instruction_decoder_shift_add.h"
#include "instruction_decoder_pack_mask.h"
#include "interface_unit.h"
#include "pc_unit.h"

#if EN_MODEL
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>

// Operation type
enum class EN_OP : uint {
    IDLE, R4_VWR, VWR_VWR, R4_R1, VWR_R1, MAX
};

const std::map<EN_OP, std::string> EN_OP_STRING = {
    { EN_OP::IDLE,      "IDLE" },
    { EN_OP::R4_VWR,    "R4_VWR" },
    { EN_OP::VWR_VWR,   "VWR_VWR" },
    { EN_OP::R4_R1,     "R4_R1" },
    { EN_OP::VWR_R1,    "VWR_R1" },
};

enum class CP_EN_OP : uint {
    IDLE, WRF_CSDLEN_LOOP, WRF_IB, WRF_CSD, NOP, RLB, WLB, VMV, SHIFT, ADD, MUL, PACK, EXIT, MAX
};

const std::map<CP_EN_OP, std::string> CP_EN_OP_STRING = {
    { CP_EN_OP::IDLE,               "IDLE" },
    { CP_EN_OP::WRF_CSDLEN_LOOP,    "WRF_CSDLEN_LOOP" },
    { CP_EN_OP::WRF_IB,             "WRF_IB" },
    { CP_EN_OP::WRF_CSD,            "WRF_CSD" },
    { CP_EN_OP::NOP,                "NOP" },
    { CP_EN_OP::RLB,                "RLB" },
    { CP_EN_OP::WLB,                "WLB" },
    { CP_EN_OP::VMV,                "VMV" },
    { CP_EN_OP::SHIFT,              "SHIFT" },
    { CP_EN_OP::ADD,                "ADD" },
    { CP_EN_OP::MUL,                "MUL" },
    { CP_EN_OP::PACK,               "PACK" },
    { CP_EN_OP::EXIT,               "EXIT" },
};
#endif

class control_unit: public sc_module {
public:
    sc_in_clk                   clk;
    sc_in<bool>                 rst;
    sc_in<bool>                 RD;                 // DRAM read command
    sc_in<bool>                 WR;                 // DRAM write command
    sc_in<bool>                 ACT;                // DRAM activate command
//    sc_in<bool>                   RSTB;               //
    sc_in<bool>                 AB_mode;            // Signals if the All-Banks mode is enabled
    sc_in<bool>                 pim_mode;           // Signals if the PIM mode is enabled
    sc_in<sc_uint<BANK_BITS> >  bank_addr;          // Address of the bank
    sc_in<sc_uint<ROW_BITS> >   row_addr;           // Address of the bank row
    sc_in<sc_uint<COL_BITS> >   col_addr;           // Address of the bank column
    sc_in<uint64_t>             DQ;                 // Data input from DRAM controller (output makes no sense)
    sc_in<uint64_t>             macroinstr;         // Macroinstruction from the IB
    sc_in<uint64_t>             csd_mult[CSD_64B];  // CSD multiplier fetched from the CSD RF
    sc_out<uint>                pc_out;             // PC for the instruction memories
    sc_out<uint64_t>            data_out;           // Data to the Registers

    // Instruction buffer control
    sc_out<bool>    ib_wr_en;       // Enables writing of a received instruction
    sc_out<uint>    ib_wr_addr;     // Index for writing the received instructions from processor

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

    // CSD RF control
    sc_out<uint>    csdrf_rd_addr;  // Index read
    sc_out<bool>    csdrf_wr_en;    // Enable writing
    sc_out<uint>    csdrf_wr_addr;  // Index the address to be written
    sc_out<MUX>     csdrf_wr_from;  // Index the MUX for input data

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

    // Internal modules
    interface_unit *iu;
    pc_unit *pcu;
    itt_mov *ittm;
    itt_sa *ittsa;
    itt_pm *ittpm;
    dlb_mov *dlbm;
    dlb_sa *dlbsa;
    dlb_pm *dlbpm;
    mult_sequencer *ms;
    instruction_decoder_mov *idm;
    instruction_decoder_shift_add *idsa;
    instruction_decoder_pack_mask *idpm;

    // Internal signals and variables
    sc_signal<bool> rf_access, decode_en;
    sc_signal<bool> pc_rst, count_en;
    sc_signal<uint> pc;
#if !HW_LOOP
    sc_signal<bool> jump_en;
    sc_signal<uint8_t> jump_num;
#else
    sc_signal<bool> loop_reg_en;
    sc_signal<uint> loop_sta_addr, loop_end_addr, loop_num_iter;
#endif
    sc_signal<bool> nop_active, idm_decode_en, idsa_decode_en, idpm_decode_en;
    sc_signal<bool> decoding_reg, decoding_nxt;

    sc_signal<uint>         mov_fields;
    sc_signal<uint>         sa_fields;
    sc_signal<uint>         pm_fields;

    sc_signal<uint>         itt_idx, itt_idx_reg, itt_idx_nxt;
    sc_signal<uint>         common_reg, common_nxt;
    sc_signal<itt_field>    ittm_field, ittsa_field, ittpm_field;
    sc_signal<uint>         dlbm_index, dlbsa_index, dlbpm_index;
    sc_signal<uint16_t>     mov_instr, sa_instr, pm_instr;

    sc_signal<bool>         ms_en, ms_sa_valid, ms_mov_valid;
    sc_signal<SWSIZE>       ms_size;
    sc_signal<OPC_STORAGE>  ms_src, ms_dst;
    sc_signal<uint>         ms_src_n, ms_dst_n;
    sc_signal<OPC_STORAGE>  ms_mov_src, ms_sa_src0, ms_sa_dst;
    sc_signal<uint>         ms_mov_src_n, ms_mov_dst_n;
    sc_signal<SWSIZE>       ms_sa_size;
    sc_signal<bool>         ms_csd_len_en;
    sc_signal<uint8_t>      csd_len;
    sc_signal<uint>         ms_sa_index;
#if (INSTR_FORMAT == BASE_FORMAT)
    sc_signal<uint>         ms_sa_shift;
#endif

    sc_signal<bool> idm_reg_en[REG_NUM], idsa_reg_en[REG_NUM], idpm_reg_en[REG_NUM];
    sc_signal<MUX>  idm_reg_from[REG_NUM], idsa_reg_from[REG_NUM], idpm_reg_from[REG_NUM];
    sc_signal<MUX>  idm_sa_op1_from, idsa_sa_op1_from;

    sc_signal<bool>         pm_out_to_vwr;

#if EN_MODEL
    // Energy model variables
    std::string filename;
    std::ofstream ef_s1, ef_s2, ef_cp;
    uint**** stats_s1 = new uint***[uint(EN_OP::MAX)];  // [EN_OP][SWSIZE][ADDOP][SA_SHIFT]
    uint* stats_s2 = new uint[uint(SWREPACK::INV)+1]();
    uint* stats_cp = new uint[uint(CP_EN_OP::MAX)]();

    SC_HAS_PROCESS(control_unit);
    control_unit(sc_module_name name, std::string filename) : sc_module(name), filename(filename) {

        // Initialize the energy model variables to zero
        for (uint i = 0; i < uint(EN_OP::MAX); i++) {
            stats_s1[i] = new uint**[uint(SWSIZE::B24)+1];
            for (uint j = 0; j < uint(SWSIZE::B24)+1; j++) {
                stats_s1[i][j] = new uint*[uint(ADDOP::MAX)];
                for (uint k = 0; k < uint(ADDOP::MAX); k++) {
                    stats_s1[i][j][k] = new uint[SA_MAX_SHIFT+1]();
                }
            }
        }
#else
    SC_HAS_PROCESS(control_unit);
    control_unit(sc_module_name name) : sc_module(name) {
#endif
        iu = new interface_unit("interface_unit");
        // Inputs
        iu->clk(clk);
        iu->rst(rst);
        iu->RD(RD);
        iu->WR(WR);
        iu->ACT(ACT);
//      iu->RSTB(RSTB);
        iu->AB_mode(AB_mode);
        iu->pim_mode(pim_mode);
        iu->bank_addr(bank_addr);
        iu->row_addr(row_addr);
        iu->col_addr(col_addr);
        iu->DQ(DQ);
        // Outputs
        iu->rf_access(rf_access);
        iu->decode_en(decode_en);
        iu->data_out(data_out);
#if HW_LOOP
        iu->loop_reg_en(loop_reg_en);
        iu->loop_sta_addr(loop_sta_addr);
        iu->loop_end_addr(loop_end_addr);
        iu->loop_num_iter(loop_num_iter);
#endif
#if STATIC_CSD_LEN
        iu->csd_len_en(ms_csd_len_en);
        iu->csd_len(csd_len);
#endif

        pcu = new pc_unit("PC_unit");
        pcu->clk(clk);
        pcu->rst(rst);
        pcu->pc_rst(pc_rst);
        pcu->count_en(count_en);
#if !HW_LOOP
        pcu->jump_en(jump_en);
        pcu->jump_num(jump_num);
#else
        pcu->loop_reg_en(loop_reg_en);
        pcu->loop_sta_addr(loop_sta_addr);
        pcu->loop_end_addr(loop_end_addr);
        pcu->loop_num_iter(loop_num_iter);
#endif
        pcu->pc_out(pc);

        ittm = new itt_mov("ITT_MOV");
        ittm->index(itt_idx);
        ittm->translation(ittm_field);

        ittsa = new itt_sa("ITT_S&A");
        ittsa->index(itt_idx);
        ittsa->translation(ittsa_field);

        ittpm = new itt_pm("ITT_P&M");
        ittpm->index(itt_idx);
        ittpm->translation(ittpm_field);

        dlbm = new dlb_mov("DLB_MOV");
        dlbm->index(dlbm_index);
        dlbm->microinstr(mov_instr);

        dlbsa = new dlb_sa("DLB_S&A");
        dlbsa->index(dlbsa_index);
        dlbsa->microinstr(sa_instr);

        dlbpm = new dlb_pm("DLB_P&M");
        dlbpm->index(dlbpm_index);
        dlbpm->microinstr(pm_instr);

        ms = new mult_sequencer("multiplication_sequencer");
        ms->clk(clk);
        ms->rst(rst);
        ms->enable(ms_en);
        ms->sw_size(ms_size);
        ms->src(ms_src);
        ms->src_n(ms_src_n);
        ms->dst(ms_dst);
        ms->dst_n(ms_dst_n);
        for (uint i = 0; i < CSD_64B; i++) {
            ms->csd_in[i](csd_mult[i]);
        }
        ms->csd_len(csd_len);
#if STATIC_CSD_LEN
        ms->csd_len_en(ms_csd_len_en);
#endif
        ms->mov_valid(ms_mov_valid);
        ms->mov_src(ms_mov_src);
        ms->mov_src_n(ms_mov_src_n);
        ms->sa_valid(ms_sa_valid);
        ms->sa_index(ms_sa_index);
        ms->sa_size(ms_sa_size);
#if (INSTR_FORMAT == BASE_FORMAT)
        ms->sa_shift(ms_sa_shift);
#endif
        ms->sa_src0(ms_sa_src0);
        ms->sa_dst(ms_sa_dst);
        ms->mov_dst_n(ms_mov_dst_n);

        idm = new instruction_decoder_mov("instruction_decoder_mov");
        idm->clk(clk);
        idm->rst(rst);
        idm->rf_access(rf_access);
        idm->decode_en(idm_decode_en);
        idm->pc_in(pc);
        idm->instr(mov_instr);
        idm->common_fields(mov_fields);
        idm->row_addr(row_addr);
        idm->col_addr(col_addr);
        idm->data_from_pm(pm_out_to_vwr);
        idm->ms_valid(ms_mov_valid);
        idm->ms_src(ms_mov_src);
        idm->ms_src_n(ms_mov_src_n);
        idm->ms_dst(ms_sa_dst);
        idm->ms_dst_n(ms_mov_dst_n);
        // Program Counter Control
        idm->pc_rst(pc_rst);
        idm->nop_active(nop_active);
#if !HW_LOOP
        idm->jump_en(jump_en);
        idm->jump_num(jump_num);
#endif
        // Instruction buffer control
        idm->ib_wr_en(ib_wr_en);
        idm->ib_wr_addr(ib_wr_addr);
        // Tile shuffler control
        idm->ts_in_en(ts_in_en);
        idm->ts_out_en(ts_out_en);
        idm->ts_out_start(ts_out_start);
        idm->ts_out_mode(ts_out_mode);
        idm->ts_shf_from(ts_shf_from);
        // VWRs control
        for (uint i = 0; i < VWR_NUM; i++) {
            idm->vwr_enable[i](vwr_enable[i]);
            idm->vwr_wr_nrd[i](vwr_wr_nrd[i]);
#if VWR_DRAM_CLK > 1
            idm->vwr_dram_d_nm[i](vwr_dram_d_nm[i]);
#endif
            idm->vwr_d_nm[i](vwr_d_nm[i]);
            idm->vwr_mask_en[i](vwr_mask_en[i]);
            idm->vwr_mask[i](vwr_mask[i]);
            idm->vwr_idx[i](vwr_idx[i]);
            idm->vwr_from[i](vwr_from[i]);
        }
#if VWR_DRAM_CLK > 1
        idm->vwr_dram_idx(vwr_dram_idx);
#endif
        // Registers control
        for (uint i = 0; i < REG_NUM; i++) {
            idm->reg_en[i](idm_reg_en[i]);
            idm->reg_from[i](idm_reg_from[i]);
        }
        // SRF control
        idm->srf_wr_en(srf_wr_en);
        idm->srf_wr_addr(srf_wr_addr);
        idm->srf_wr_from(srf_wr_from);
        // Mask RF control
        idm->mrf_wr_en(mrf_wr_en);
        idm->mrf_wr_addr(mrf_wr_addr);
        idm->mrf_wr_from(mrf_wr_from);
        // CSD RF control
        idm->csdrf_wr_en(csdrf_wr_en);
        idm->csdrf_wr_addr(csdrf_wr_addr);
        idm->csdrf_wr_from(csdrf_wr_from);
        // Interface with SA
        idm->sa_src0_from(idm_sa_op1_from);
#if DUAL_BANK_INTERFACE
        // BANKS Control
        idm->even_out_en(even_out_en);
        idm->odd_out_en(odd_out_en);
#else
        idm->dram_out_en(dram_out_en);
        idm->dram_from(dram_from);
#endif

        idsa = new instruction_decoder_shift_add("instruction_decoder_sa");
        idsa->clk(clk);
        idsa->rst(rst);
        idsa->decode_en(idsa_decode_en);
        idsa->instr(sa_instr);
        idsa->common_fields(sa_fields);
        idsa->ms_valid(ms_sa_valid);
        idsa->ms_size(ms_sa_size);
#if (INSTR_FORMAT == BASE_FORMAT)
        idsa->ms_shift(ms_sa_shift);
#endif
        idsa->ms_src0(ms_sa_src0);
        idsa->ms_dst(ms_sa_dst);
        // Registers control
        for (uint i = 0; i < REG_NUM; i++) {
            idsa->reg_en[i](idsa_reg_en[i]);
            idsa->reg_from[i](idsa_reg_from[i]);
        }
        // SRF control
        idsa->srf_rd_addr(srf_rd_addr);
        // Shift and Add control
        idsa->sa_en(sa_en);
        idsa->sa_shift(sa_shift);
        idsa->sa_size(sa_size);
        idsa->sa_adder_en(sa_adder_en);
        idsa->sa_neg_op1(sa_neg_op1);
        idsa->sa_neg_op2(sa_neg_op2);
        idsa->sa_op1_from(idsa_sa_op1_from);
        idsa->sa_op2_from(sa_op2_from);

        idpm = new instruction_decoder_pack_mask("instruction_decoder_pm");
        idpm->clk(clk);
        idpm->rst(rst);
        idpm->decode_en(idpm_decode_en);
        idpm->instr(pm_instr);
        idpm->common_fields(pm_fields);
        // Registers control
        for (uint i = 0; i < REG_NUM; i++) {
            idpm->reg_en[i](idpm_reg_en[i]);
            idpm->reg_from[i](idpm_reg_from[i]);
        }
        // Mask RF control
        idpm->mrf_rd_addr(mrf_rd_addr);
        // Pack and Mask control
        idpm->pm_en(pm_en);
        idpm->pm_repack(pm_repack);
        idpm->pm_in_start(pm_in_start);
        idpm->pm_op_sel(pm_op_sel);
        idpm->pm_shift(pm_shift);
        idpm->pm_op1_from(pm_op1_from);
        idpm->pm_op2_from(pm_op2_from);
        idpm->pm_out_to_vwr(pm_out_to_vwr);

        SC_THREAD(clk_thread);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);

        SC_METHOD(comb_method);
        sensitive << decoding_reg << decode_en << nop_active;
        sensitive << itt_idx_reg << macroinstr << common_reg;
        sensitive << ittm_field << ittsa_field << ittpm_field;
        sensitive << ms_mov_valid << ms_mov_src << ms_mov_src_n << ms_mov_dst_n;
        sensitive << ms_sa_valid << ms_sa_index << ms_sa_dst;
#if (INSTR_FORMAT == BASE_FORMAT)
        sensitive << ms_sa_shift;
#endif
        sensitive << pm_repack << idm_sa_op1_from << idsa_sa_op1_from;

        SC_METHOD(output_method);
        sensitive << pc;
        for (uint i=0; i<REG_NUM; i++) {
            sensitive << idm_reg_en[i] << idsa_reg_en[i] << idpm_reg_en[i];
            sensitive << idm_reg_from[i] << idsa_reg_from[i] << idpm_reg_from[i];
        }

#if EN_MODEL
        SC_THREAD(energy_thread);
        sensitive << clk.pos();
#endif
    }

    void clk_thread();
    void comb_method();
    void output_method();
#if EN_MODEL
    void energy_thread();
    void write_energy_stats();
#endif
};

#endif /* SRC_CONTROL_UNIT_H_ */
