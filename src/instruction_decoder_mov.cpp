/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Implementation of the Instruction Decoder for Movement instructions.
 *
 */

#include "instruction_decoder_mov.h"

void instruction_decoder_mov::clk_thread() {

    // Reset all registers and pipelines
    nop_cnt_reg = 0;

#if !HW_LOOP
    jmp_act_reg = false;
    jmp_cnt_reg = false;
#endif

    wait();

    // Update registers and advance pipelines
    while (1) {
        nop_cnt_reg = nop_cnt_nxt;

#if !HW_LOOP
        jmp_act_reg = jmp_act_nxt;
        jmp_cnt_reg = jmp_cnt_nxt;
#endif

        wait();
    }
}

void instruction_decoder_mov::comb_method() {

    uint16_t microinstr = instr->read();
    uint32_t fields = common_fields->read();

    MOP mop;
    OPC_STORAGE dst, src, dst_sa;
    uint imm, dst_n_sa;

    // ** DEFAULT VALUES FOR SIGNALS **
    if (nop_cnt_reg == 0) {
        nop_cnt_nxt = 0;
        nop_active->write(false);
    } else {
        nop_cnt_nxt = nop_cnt_reg - 1;
        nop_active->write(true);
    }

#if !HW_LOOP
    jmp_act_nxt = jmp_act_reg;
    jmp_cnt_nxt = jmp_cnt_reg;
    jump_en->write(false);
    jump_num->write(0);
#endif

    // Ports
    pc_rst->write(false);
#if !HW_LOOP
    jump_en->write(false);
    jump_num->write(0);
#endif
    ib_wr_en->write(false);
    ib_wr_addr->write(0);

    ts_in_en->write(false);
    ts_out_en->write(false);
    ts_out_start->write(0);
    ts_out_mode->write(TS_MODE::NOP);
    ts_shf_from->write(MUX::EXT);

    for (uint i = 0; i < VWR_NUM; i++) {
        vwr_enable[i]->write(false);
        vwr_wr_nrd[i]->write(false);
#if VWR_DRAM_CLK > 1
        vwr_dram_d_nm[i]->write(false));
#endif
        vwr_d_nm[i]->write(false);
        vwr_mask_en[i]->write(false);
        vwr_mask[i]->write(0);
        vwr_idx[i]->write(0);
        vwr_from[i]->write(MUX::EXT);
    }
#if VWR_DRAM_CLK > 1
    vwr_dram_idx->write(0);
#endif

    for (uint i = 0; i < REG_NUM; i++) {
        reg_en[i]->write(false);
        reg_from[i]->write(MUX::EXT);
    }

    sa_src0_from->write(MUX::VWR0);

    srf_wr_en->write(false);
    srf_wr_from->write(MUX::EXT);
    srf_wr_addr->write(0);

    mrf_wr_en->write(false);
    mrf_wr_from->write(MUX::EXT);
    mrf_wr_addr->write(0);

    csdrf_wr_en->write(false);
    csdrf_wr_from->write(MUX::EXT);
    csdrf_wr_addr->write(0);

#if DUAL_BANK_INTERFACE
    even_out_en->write(false);
    odd_out_en->write(false);
#else
    dram_out_en->write(false);
    dram_from->write(MUX::EXT);
#endif

    // Intermediate variables
    MUX rd_from_mux = MUX::EXT;
    sc_uint<ROW_BITS> row = row_addr->read(); // Separate row address in MSB and rest
    sc_uint<ROW_BITS> col = col_addr->read();
    sc_uint<ROW_BITS - 1> rlsb = row.range(ROW_BITS - 2, 0);
    sc_uint<ROW_BITS - 1 + COL_BITS> rowcol_addr;
    rowcol_addr.range(ROW_BITS - 1 + COL_BITS - 1, COL_BITS) = rlsb;
    rowcol_addr.range(COL_BITS - 1, 0) = col;
    sc_uint<RF_SEL_BITS> rf_sel = rowcol_addr.range(RF_SEL_BITS + RF_ADDR_BITS - 1, RF_ADDR_BITS);
    sc_uint<RF_ADDR_BITS> rf_addr = rowcol_addr.range(RF_ADDR_BITS - 1, 0);
    sc_uint<MC_COL_BITS> multicycle_idx = col.range(MC_COL_BITS - 1, 0);
    TS_MODE ts_mode_aux;

    // Write to RFs from host   // TODO still to check access between DRAM and VWRs
    // Signal width adaptation in the interface unit
    if (rf_access->read()) {
        switch (RF_SEL(uint(rf_sel))) {
            case RF_SEL::IB:
                ib_wr_en->write(true);
                ib_wr_addr->write(rf_addr.to_uint());
            break;
            case RF_SEL::SRF:
                srf_wr_en->write(true);
                srf_wr_addr->write(rf_addr.to_uint());
                srf_wr_from->write(MUX::EXT);
            break;
            case RF_SEL::MRF:
                mrf_wr_en->write(true);
                mrf_wr_addr->write(rf_addr.to_uint());
                mrf_wr_from->write(MUX::EXT);
            break;
            case RF_SEL::CSDRF:
                csdrf_wr_en->write(true);
                csdrf_wr_addr->write(rf_addr.to_uint());
                csdrf_wr_from->write(MUX::EXT);
            break;
            case RF_SEL::VWR:
//                if (rf_addr.to_uint() < VWR_NUM) {
//                    vwr_enable[rf_addr.to_uint()]->write(true);
//                    vwr_wr_nrd[rf_addr.to_uint()]->write(true);
//                    vwr_d_nm[rf_addr.to_uint()]->write(false);
//                    vwr_from[rf_addr.to_uint()]->write(MUX::EXT);
//                }
            break;
            default:
            break;
        }
    }

    // Decode new instruction when signaled (if any)
    if (decode_en->read()) {

        decode_mov_microinstr(microinstr, fields, &mop, &dst, &src, &imm, &dst_sa, &dst_n_sa);  // Decode the microinstruction + fields from macro
        if (data_from_pm)   src = OPC_STORAGE::PM;

        if (ms_valid->read()) {
            if (is_vwr(ms_dst->read())) {
                mop = MOP::MOV_SAWB;
                dst = OPC_STORAGE::SA;
            } else if (is_vwr(ms_src->read())) {
                mop = MOP::MOV;
                dst = OPC_STORAGE::SAR0;
            }
            src = ms_src->read();
            imm = ms_src_n->read();
            dst_sa = ms_dst->read();
            dst_n_sa = ms_dst_n->read();
        }

        // Decode the instruction and generate the control signals
        switch (mop) {

            // NOP
            case MOP::NOP:
                nop_cnt_nxt = imm - 1;
            break;

            // Reset PC
            case MOP::EXIT:
                pc_rst->write(true);
            break;

#if !HW_LOOP
            // JUMP with zero-cycles execution stage and only one register for number of loops
            case MOP::JUMP:
                if (!jmp_act_reg) {         // First sight of this jump
                    jmp_act_nxt = true;
                    jmp_cnt_nxt = IMM1 - 1;
                    jump_en->write(true);
                    jump_num->write(IMM0);
                } else if (jmp_cnt_reg) {   // Still more jumps to make
                    jmp_act_nxt = true;
                    jmp_cnt_nxt = jmp_cnt_reg - 1;
                    jump_en->write(true);
                    jump_num->write(IMM0);
                } else {                    // Last jump made
                    jmp_act_nxt = false;
                }
            break;
#endif

            case MOP::DRAM_RD:  // Assume in this cycle the bank already pushed the data to IO
                switch (dst) {
#if DUAL_BANK_INTERFACE
                    case OPC_STORAGE::VWR0:
                        vwr_enable[0]->write(true);
                        vwr_wr_nrd[0]->write(true);
                        vwr_mask_en[0]->write(true);
                        vwr_mask[0]->write(imm);
#if VWR_DRAM_CLK > 1
                        vwr_dram_d_nm[0]->write(true);
                        vwr_dram_idx->write(multicycle_idx.to_uint());
#endif
                        vwr_d_nm[0]->write(false);
                        vwr_from[0]->write(MUX::EVEN_BANK);
                    break;
                    case OPC_STORAGE::VWR1:
                        vwr_enable[1]->write(true);
                        vwr_wr_nrd[1]->write(true);
                        vwr_mask_en[1]->write(true);
                        vwr_mask[1]->write(imm);
#if VWR_DRAM_CLK > 1
                        vwr_dram_d_nm[1]->write(true);
                        vwr_dram_idx->write(multicycle_idx.to_uint());
#endif
                        vwr_d_nm[1]->write(false);
                        vwr_from[1]->write(MUX::EVEN_BANK);
                    break;
#else
                    case OPC_STORAGE::VWR0:
                        vwr_enable[0]->write(true);
                        vwr_wr_nrd[0]->write(true);
#if VWR_DRAM_CLK > 1
                        vwr_dram_d_nm[0]->write(true);
                        vwr_dram_idx->write(multicycle_idx.to_uint());
#endif
                        vwr_mask_en[0]->write(true);
                        vwr_mask[0]->write(imm);
                        vwr_d_nm[0]->write(false);
                        vwr_from[0]->write(MUX::DRAM);
                    break;
                    case OPC_STORAGE::VWR1:
                        vwr_enable[1]->write(true);
                        vwr_wr_nrd[1]->write(true);
#if VWR_DRAM_CLK > 1
                        vwr_dram_d_nm[1]->write(true);
                        vwr_dram_idx->write(multicycle_idx.to_uint());
#endif
                        vwr_mask_en[1]->write(true);
                        vwr_mask[1]->write(imm);
                        vwr_d_nm[1]->write(false);
                        vwr_from[1]->write(MUX::DRAM);
                    break;
#if VWR_NUM > 2
                    case OPC_STORAGE::VWR2:
                        vwr_enable[2]->write(true);
                        vwr_wr_nrd[2]->write(true);
#if VWR_DRAM_CLK > 1
                        vwr_dram_d_nm[2]->write(true);
                        vwr_dram_idx->write(multicycle_idx.to_uint());
#endif
                        vwr_mask_en[2]->write(true);
                        vwr_mask[2]->write(imm);
                        vwr_d_nm[2]->write(false);
                        vwr_from[2]->write(MUX::DRAM);
                    break;
#if VWR_NUM > 3
                    case OPC_STORAGE::VWR3:
                        vwr_enable[3]->write(true);
                        vwr_wr_nrd[3]->write(true);
#if VWR_DRAM_CLK > 1
                        vwr_dram_d_nm[3]->write(true);
                        vwr_dram_idx->write(multicycle_idx.to_uint());
#endif
                        vwr_mask_en[3]->write(true);
                        vwr_mask[3]->write(imm);
                        vwr_d_nm[3]->write(false);
                        vwr_from[3]->write(MUX::DRAM);
                    break;
#endif
#endif
#endif  // DUAL_BANK_INTERFACE
                    default:
                    break;
                }
            break;

            case MOP::DRAM_WR:
                switch (src) {
#if DUAL_BANK_INTERFACE
                    case OPC_STORAGE::VWR0:
                        vwr_enable[0]->write(true);
                        vwr_wr_nrd[0]->write(false);
                        vwr_d_nm[0]->write(true);
                        vwr_mask_en[0]->write(true);
                        vwr_mask[0]->write(imm);
#if VWR_DRAM_CLK > 1
                        vwr_dram_d_nm[0]->write(false);
                        vwr_dram_idx->write(multicycle_idx.to_uint());
#endif
                        even_out_en->write(true));
                    break;
                    case OPC_STORAGE::VWR1:
                        vwr_enable[1]->write(true);
                        vwr_wr_nrd[1]->write(true);
                        vwr_d_nm[1]->write(false);
                        vwr_mask_en[1]->write(true);
                        vwr_mask[1]->write(imm);
#if VWR_DRAM_CLK > 1
                        vwr_dram_d_nm[1]->write(false);
                        vwr_dram_idx->write(multicycle_idx.to_uint());
#endif
                        odd_out_en->write(true));
                    break;
#else
                    case OPC_STORAGE::VWR0:
                        vwr_enable[0]->write(true);
                        vwr_wr_nrd[0]->write(false);
                        vwr_d_nm[0]->write(true);
                        vwr_mask_en[0]->write(true);
                        vwr_mask[0]->write(imm);
#if VWR_DRAM_CLK > 1
                        vwr_dram_d_nm[0]->write(false);
                        vwr_dram_idx->write(multicycle_idx.to_uint());
#endif
                        dram_out_en->write(true);
                        dram_from->write(MUX::VWR0);
                    break;
                    case OPC_STORAGE::VWR1:
                        vwr_enable[1]->write(true);
                        vwr_wr_nrd[1]->write(false);
                        vwr_d_nm[1]->write(true);
                        vwr_mask_en[1]->write(true);
                        vwr_mask[1]->write(imm);
#if VWR_DRAM_CLK > 1
                        vwr_dram_d_nm[1]->write(false);
                        vwr_dram_idx->write(multicycle_idx.to_uint());
#endif
                        dram_out_en->write(true);
                        dram_from->write(MUX::VWR1);
                    break;
#if VWR_NUM > 2
                    case OPC_STORAGE::VWR2:
                        vwr_enable[2]->write(true);
                        vwr_wr_nrd[2]->write(false);
                        vwr_d_nm[2]->write(true);
                        vwr_mask_en[2]->write(true);
                        vwr_mask[2]->write(imm);
#if VWR_DRAM_CLK > 1
                        vwr_dram_d_nm[2]->write(false);
                        vwr_dram_idx->write(multicycle_idx.to_uint());
#endif
                        dram_out_en->write(true);
                        dram_from->write(MUX::VWR2);
                    break;
#if VWR_NUM > 3
                    case OPC_STORAGE::VWR3:
                        vwr_enable[3]->write(true);
                        vwr_wr_nrd[3]->write(false);
                        vwr_d_nm[3]->write(true);
                        vwr_mask_en[3]->write(true);
                        vwr_mask[3]->write(imm);
#if VWR_DRAM_CLK > 1
                        vwr_dram_d_nm[3]->write(false);
                        vwr_dram_idx->write(multicycle_idx.to_uint());
#endif
                        dram_out_en->write(true);
                        dram_from->write(MUX::VWR3);
                    break;
#endif
#endif
#endif // DUAL_BANK_INTERFACE
                    default:
                    break;
                }
            break;

            //TODO should we add ReLU? Maybe as extra
            case MOP::MOV:
                switch (src) {
                    case OPC_STORAGE::R3:
                        rd_from_mux = MUX::R3;
                    break;
                    case OPC_STORAGE::VWR0:
                        vwr_enable[0]->write(true);
                        vwr_wr_nrd[0]->write(false);
                        vwr_d_nm[0]->write(false);
                        vwr_idx[0]->write(imm);
                        rd_from_mux = MUX::VWR0;
                    break;
                    case OPC_STORAGE::VWR1:
                        vwr_enable[1]->write(true);
                        vwr_wr_nrd[1]->write(false);
                        vwr_d_nm[1]->write(false);
                        vwr_idx[1]->write(imm);
                        rd_from_mux = MUX::VWR1;
                    break;
#if VWR_NUM > 2
                    case OPC_STORAGE::VWR2:
                        vwr_enable[2]->write(true);
                        vwr_wr_nrd[2]->write(false);
                        vwr_d_nm[2]->write(false);
                        vwr_idx[2]->write(imm);
                        rd_from_mux = MUX::VWR2;
                    break;
#if VWR_NUM > 3
                    case OPC_STORAGE::VWR3:
                        vwr_enable[3]->write(true);
                        vwr_wr_nrd[3]->write(false);
                        vwr_d_nm[3]->write(false);
                        vwr_idx[3]->write(imm);
                        rd_from_mux = MUX::VWR3;
                    break;
#endif
#endif
                    case OPC_STORAGE::SA:
                        rd_from_mux = MUX::SA;
                    break;
                    case OPC_STORAGE::PM:
                        rd_from_mux = MUX::PM;
                    break;
                    default:
                    break;
                }
                switch (dst) {
                    case OPC_STORAGE::R0:
                        reg_en[0]->write(true);
                        reg_from[0]->write(rd_from_mux);
                    break;
                    case OPC_STORAGE::VWR0:
                        vwr_enable[0]->write(true);
                        vwr_wr_nrd[0]->write(true);
                        vwr_d_nm[0]->write(true);
                        vwr_idx[0]->write(imm);
                        vwr_from[0]->write(rd_from_mux);
                    break;
                    case OPC_STORAGE::VWR1:
                        vwr_enable[1]->write(true);
                        vwr_wr_nrd[1]->write(true);
                        vwr_d_nm[1]->write(true);
                        vwr_idx[1]->write(imm);
                        vwr_from[1]->write(rd_from_mux);
                    break;
#if VWR_NUM > 2
                    case OPC_STORAGE::VWR2:
                        vwr_enable[2]->write(true);
                        vwr_wr_nrd[2]->write(true);
                        vwr_d_nm[2]->write(true);
                        vwr_idx[2]->write(imm);
                        vwr_from[2]->write(rd_from_mux);
                    break;
#if VWR_NUM > 3
                    case OPC_STORAGE::VWR3:
                        vwr_enable[3]->write(true);
                        vwr_wr_nrd[3]->write(true);
                        vwr_d_nm[3]->write(true);
                        vwr_idx[3]->write(imm);
                        vwr_from[3]->write(rd_from_mux);
                    break;
#endif
#endif
                    case OPC_STORAGE::SA:
                        sa_src0_from->write(rd_from_mux);
                    break;
                    case OPC_STORAGE::SAR0:
                        sa_src0_from->write(rd_from_mux);
                        reg_en[0]->write(true);
                        reg_from[0]->write(rd_from_mux);
                    break;
                    default:
                    break;
                }
            break;

            case MOP::MOV_SAWB:
                switch (src) {
                    case OPC_STORAGE::R3:
                        rd_from_mux = MUX::R3;
                    break;
                    case OPC_STORAGE::VWR0:
                        vwr_enable[0]->write(true);
                        vwr_wr_nrd[0]->write(false);
                        vwr_d_nm[0]->write(false);
                        vwr_idx[0]->write(imm);
                        rd_from_mux = MUX::VWR0;
                    break;
                    case OPC_STORAGE::VWR1:
                        vwr_enable[1]->write(true);
                        vwr_wr_nrd[1]->write(false);
                        vwr_d_nm[1]->write(false);
                        vwr_idx[1]->write(imm);
                        rd_from_mux = MUX::VWR1;
                    break;
#if VWR_NUM > 2
                    case OPC_STORAGE::VWR2:
                        vwr_enable[2]->write(true);
                        vwr_wr_nrd[2]->write(false);
                        vwr_d_nm[2]->write(false);
                        vwr_idx[2]->write(imm);
                        rd_from_mux = MUX::VWR2;
                    break;
#if VWR_NUM > 3
                    case OPC_STORAGE::VWR3:
                        vwr_enable[3]->write(true);
                        vwr_wr_nrd[3]->write(false);
                        vwr_d_nm[3]->write(false);
                        vwr_idx[3]->write(imm);
                        rd_from_mux = MUX::VWR3;
                    break;
#endif
#endif
                    case OPC_STORAGE::SA:
                        rd_from_mux = MUX::SA;
                    break;
                    case OPC_STORAGE::PM:
                        rd_from_mux = MUX::PM;
                    break;
                    default:
                    break;
                }
                switch (dst) {
                    case OPC_STORAGE::R0:
                        reg_en[0]->write(true);
                        reg_from[0]->write(rd_from_mux);
                    break;
                    case OPC_STORAGE::VWR0:
                        vwr_enable[0]->write(true);
                        vwr_wr_nrd[0]->write(true);
                        vwr_d_nm[0]->write(true);
                        vwr_idx[0]->write(imm);
                        vwr_from[0]->write(rd_from_mux);
                    break;
                    case OPC_STORAGE::VWR1:
                        vwr_enable[1]->write(true);
                        vwr_wr_nrd[1]->write(true);
                        vwr_d_nm[1]->write(true);
                        vwr_idx[1]->write(imm);
                        vwr_from[1]->write(rd_from_mux);
                    break;
#if VWR_NUM > 2
                    case OPC_STORAGE::VWR2:
                        vwr_enable[2]->write(true);
                        vwr_wr_nrd[2]->write(true);
                        vwr_d_nm[2]->write(true);
                        vwr_idx[2]->write(imm);
                        vwr_from[2]->write(rd_from_mux);
                    break;
#if VWR_NUM > 3
                    case OPC_STORAGE::VWR3:
                        vwr_enable[3]->write(true);
                        vwr_wr_nrd[3]->write(true);
                        vwr_d_nm[3]->write(true);
                        vwr_idx[3]->write(imm);
                        vwr_from[3]->write(rd_from_mux);
                    break;
#endif
#endif
                    case OPC_STORAGE::SA:
                        sa_src0_from->write(rd_from_mux);
                    break;
                    default:
                    break;
                }
                // Perform the writeback from SA to VWR
                if (dst_sa != dst) {
                    switch (dst_sa) {
                        case OPC_STORAGE::VWR0:
                            vwr_enable[0]->write(true);
                            vwr_wr_nrd[0]->write(true);
                            vwr_d_nm[0]->write(true);
                            vwr_idx[0]->write(dst_n_sa);
                            vwr_from[0]->write(MUX::SA);
                        break;
                        case OPC_STORAGE::VWR1:
                            vwr_enable[1]->write(true);
                            vwr_wr_nrd[1]->write(true);
                            vwr_d_nm[1]->write(true);
                            vwr_idx[1]->write(dst_n_sa);
                            vwr_from[1]->write(MUX::SA);
                        break;
#if VWR_NUM > 2
                        case OPC_STORAGE::VWR2:
                            vwr_enable[2]->write(true);
                            vwr_wr_nrd[2]->write(true);
                            vwr_d_nm[2]->write(true);
                            vwr_idx[2]->write(dst_n_sa);
                            vwr_from[2]->write(MUX::SA);
                        break;
#if VWR_NUM > 3
                        case OPC_STORAGE::VWR3:
                            vwr_enable[3]->write(true);
                            vwr_wr_nrd[3]->write(true);
                            vwr_d_nm[3]->write(true);
                            vwr_idx[3]->write(dst_n_sa);
                            vwr_from[3]->write(MUX::SA);
                        break;
#endif
#endif
                        default:
                        break;
                    }
                }
            break;

            case MOP::TS_FILL:
                ts_in_en->write(true);
                ts_mode_aux = (TS_MODE) ((imm >> TS_STA_BITS) & ((1 << TS_MODE_BITS) - 1));
                ts_out_mode->write(ts_mode_aux);
                ts_out_start->write((imm & ((1 << TS_STA_BITS) - 1)));
                switch (src) {
                    case OPC_STORAGE::VWR0:
                        ts_shf_from->write(MUX::VWR0);
                        vwr_enable[0]->write(true);
                        vwr_wr_nrd[0]->write(false);
                        vwr_d_nm[0]->write(true);
                    break;
                    case OPC_STORAGE::VWR1:
                        ts_shf_from->write(MUX::VWR1);
                        vwr_enable[1]->write(true);
                        vwr_wr_nrd[1]->write(false);
                        vwr_d_nm[1]->write(true);
                    break;
#if VWR_NUM > 2
                    case OPC_STORAGE::VWR2:
                        ts_shf_from->write(MUX::VWR2);
                        vwr_enable[2]->write(true);
                        vwr_wr_nrd[2]->write(false);
                        vwr_d_nm[2]->write(true);
                    break;
#if VWR_NUM > 3
                    case OPC_STORAGE::VWR3:
                        ts_shf_from->write(MUX::VWR3);
                        vwr_enable[3]->write(true);
                        vwr_wr_nrd[3]->write(false);
                        vwr_d_nm[3]->write(true);
                    break;
#endif
#endif
                    default:
                        ts_in_en->write(false);  // If not from VWR, do nothing
                    break;
                }
            break;

#if DRAM_SHUFFLE
            case MOP::TS_FILL_DRAM:
                ts_in_en->write(true);
                ts_mode_aux = (TS_MODE) ((imm >> TS_STA_BITS) & ((1 << TS_MODE_BITS) - 1));
                ts_out_mode->write(ts_mode_aux);
                ts_out_start->write((imm & ((1 << TS_STA_BITS) - 1)));
                ts_shf_from->write(MUX::DRAM);
            break;
#endif

            case MOP::TS_RLS:
                ts_out_en->write(true);
                switch (dst) {
                    case OPC_STORAGE::VWR0:
                        vwr_enable[0]->write(true);
                        vwr_wr_nrd[0]->write(true);
                        vwr_d_nm[0]->write(false);
                        vwr_from[0]->write(MUX::TILESH);
                    break;
                    case OPC_STORAGE::VWR1:
                        vwr_enable[1]->write(true);
                        vwr_wr_nrd[1]->write(true);
                        vwr_d_nm[1]->write(false);
                        vwr_from[1]->write(MUX::TILESH);
                    break;
                    case OPC_STORAGE::VWR2:
                        vwr_enable[2]->write(true);
                        vwr_wr_nrd[2]->write(true);
                        vwr_d_nm[2]->write(false);
                        vwr_from[2]->write(MUX::TILESH);
                    break;
                    case OPC_STORAGE::VWR3:
                        vwr_enable[3]->write(true);
                        vwr_wr_nrd[3]->write(true);
                        vwr_d_nm[3]->write(false);
                        vwr_from[3]->write(MUX::TILESH);
                    break;
                    default:
                        ts_out_en->write(false); // If not to VWR, do nothing
                    break;
                }
            break;

            default:
            break;
        }
    }

}

