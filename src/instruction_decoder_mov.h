/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the Instruction Decoder for Movement instructions.
 *
 */

#ifndef SRC_INSTRUCTION_DECODER_MOV_H_
#define SRC_INSTRUCTION_DECODER_MOV_H_

#include "systemc.h"

#include "cnm_base.h"

class instruction_decoder_mov: public sc_module {
public:
    sc_in_clk                   clk;
    sc_in<bool>                 rst;
    sc_in<bool>                 rf_access;      // Enables access to the RFs directly from the host
    sc_in<bool>                 decode_en;      // Enables decoding of the next instruction
    sc_in<uint>                 pc_in;          // Program Counter
    sc_in<uint16_t>             instr;          // Instruction input from DLB
    sc_in<uint32_t>             common_fields;  // Common fields from macroinstruction buffer
    sc_in<sc_uint<ROW_BITS> >   row_addr;       // Address of the bank row
    sc_in<sc_uint<COL_BITS> >   col_addr;       // Address of the bank column
    sc_in<bool>                 data_from_pm;   // Receiving data from the PM stage
    sc_in<bool>                 ms_valid;       // Signal commands from multiplication sequencer are valid
    sc_in<OPC_STORAGE>          ms_src;         // SRC coming from multiplication sequencer
    sc_in<uint>                 ms_src_n;       // SRC index coming from multiplication sequencer
    sc_in<OPC_STORAGE>          ms_dst;         // DST coming from multiplication sequencer
    sc_in<uint>                 ms_dst_n;       // DST index coming from multiplication sequencer

    // Program Counter Control
    sc_out<bool>    pc_rst;     // Synchronous reset when EXIT instruction
    sc_out<bool>    nop_active; // Signals if a multi-cycle NOP is currently active
#if !HW_LOOP
    sc_out<bool>    jump_en;    // Signals a jump which will translate into a decrease of PC
    sc_out<uint8_t> jump_num;   // Amount to subtract from the PC when doing a jump
#endif

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
    sc_out<bool>    srf_wr_en;      // Enable writing
    sc_out<uint>    srf_wr_addr;    // Index the address to be written
    sc_out<MUX>     srf_wr_from;    // Index the MUX for input data

    // Mask RF control
    sc_out<bool>    mrf_wr_en;      // Enable writing
    sc_out<uint>    mrf_wr_addr;    // Index the address to be written
    sc_out<MUX>     mrf_wr_from;    // Index the MUX for input data

    // CSD RF control
    sc_out<bool>    csdrf_wr_en;    // Enable writing
    sc_out<uint>    csdrf_wr_addr;  // Index the address to be written
    sc_out<MUX>     csdrf_wr_from;  // Index the MUX for input data

    // Interface with SA
    sc_out<MUX>     sa_src0_from;   // Selects where the info to SA src0 comes from

#if DUAL_BANK_INTERFACE
    // BANKS Control
    sc_out<bool> even_out_en;   // Enables the even bank tri-state buffer
    sc_out<bool> odd_out_en;    // Enables the odd bank tri-state buffer
#else
    sc_out<bool>    dram_out_en;    // Enables the DRAM tri-state buffer
    sc_out<MUX>     dram_from;      // Index the MUX for input data
#endif

    // ** INTERNAL SIGNALS AND VARIABLES **

    // NOP signals and variables
    sc_signal<uint8_t> nop_cnt_nxt, nop_cnt_reg;

#if !HW_LOOP
    // Jump signals and variables
    sc_signal<bool> jmp_act_nxt, jmp_act_reg;
    sc_signal<uint8_t> jmp_cnt_nxt, jmp_cnt_reg;
#endif

//    // Tile shuffler signals, variables and pipelines
//    sc_signal<bool> ts_in_en_comb;
//    sc_signal<bool> ts_out_en_nxt, ts_out_en_reg;
//    sc_signal<uint> ts_out_start_comb;
//    sc_signal<uint> ts_shf_from_comb, ts_shf_from_nxt, ts_shf_from_reg;
//
//    // VWRs signals, variables and pipelines
//    sc_signal<bool>     vwr_enable_comb[VWR_NUM], vwr_enable_nxt[VWR_NUM], vwr_enable_reg[VWR_NUM];
//    sc_signal<bool>     vwr_wr_nrd_comb[VWR_NUM], vwr_wr_nrd_nxt[VWR_NUM], vwr_wr_nrd_reg[VWR_NUM];
//#if VWR_DRAM_CLK > 1
//    sc_signal<bool>     vwr_dram_d_nm_comb[VWR_NUM], vwr_dram_d_nm_nxt[VWR_NUM], vwr_dram_d_nm_reg[VWR_NUM];
//    sc_signal<bool>     vwr_dram_idx_comb, vwr_dram_idx_nxt, vwr_dram_idx_reg;
//#endif
//    sc_signal<bool>     vwr_d_nm_comb[VWR_NUM], vwr_d_nm_nxt[VWR_NUM], vwr_d_nm_reg[VWR_NUM];
//    sc_signal<uint>     vwr_idx_comb[VWR_NUM], vwr_idx_nxt[VWR_NUM], vwr_idx_reg[VWR_NUM];
//    sc_signal<uint8_t>  vwr_from_comb[VWR_NUM], vwr_from_nxt[VWR_NUM], vwr_from_reg[VWR_NUM];

//    // Registers  signals, variables and pipelines
//    sc_signal<bool>     reg_en_comb[REG_NUM], reg_en_nxt[REG_NUM], reg_en_reg[REG_NUM];
//    sc_signal<uint8_t>  reg_from_comb[REG_NUM], reg_from_nxt[REG_NUM], reg_from_reg[REG_NUM];
//
//    // SRF signals, variables and pipelines
//    sc_signal<uint>     srf_rd_addr_comb, srf_rd_addr_nxt, srf_rd_addr_reg;
//    sc_signal<bool>     srf_wr_en_comb, srf_wr_en_nxt, srf_wr_en_reg;
//    sc_signal<uint8_t>  srf_wr_from_comb, srf_wr_from_nxt, srf_wr_from_reg;
//    sc_signal<uint>     srf_wr_addr_comb, srf_wr_addr_nxt, srf_wr_addr_reg;
//
//    // Mask RF signals, variables and pipelines
//    sc_signal<uint>     mrf_rd_addr_comb, mrf_rd_addr_nxt, mrf_rd_addr_reg;
//    sc_signal<bool>     mrf_wr_en_comb, mrf_wr_en_nxt, mrf_wr_en_reg;
//    sc_signal<uint8_t>  mrf_wr_from_comb, mrf_wr_from_nxt, mrf_wr_from_reg;
//    sc_signal<uint>     mrf_wr_addr_comb, mrf_wr_addr_nxt, mrf_wr_addr_reg;

    SC_CTOR(instruction_decoder_mov) {

        SC_THREAD(clk_thread);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);

        SC_METHOD(comb_method);
        sensitive << instr << common_fields << rf_access << decode_en << row_addr << col_addr;
        sensitive << nop_cnt_reg << data_from_pm << ms_valid << ms_src << ms_src_n << ms_dst << ms_dst_n;
#if !HW_LOOP
        sensitive << jmp_act_reg << jmp_cnt_reg;
#endif
    }

    void clk_thread();  // Performs sequential logic (and resets)
    void comb_method(); // Performs the combinational logic
};

#endif /* SRC_INSTRUCTION_DECODER_MOV_H_ */
