/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Implementation of the Control Unit for SoftSIMD.
 *
 */

#include "control_unit.h"

void control_unit::clk_thread() {
    // Reset
    itt_idx_reg = uint(MACRO_IDX::SAFE_STATE);
    common_reg = 0;
    decoding_reg = false;

    wait();

    // Clocked behaviour
    while (1) {
        itt_idx_reg = itt_idx_nxt;
        common_reg = common_nxt;
        decoding_reg = decoding_nxt;

        wait();
    }
}

void control_unit::comb_method() {

    itt_field itt_aux;
    bool mov_valid, sa_valid, pm_valid;
    uint mov_fields_aux, sa_fields_aux, pm_fields_aux;

    // Default values
    uint itt_idx_temp = itt_idx_reg;
    uint common_temp = common_reg;
    decoding_nxt = decoding_reg;
    count_en = false;
    itt_idx_nxt = uint(MACRO_IDX::SAFE_STATE);

    // Separate macroinstruction index + common_fields
    uint macro_itt_idx = (macroinstr->read() >> SHIFT_ITT_IDX) & ((1 << ITT_IDX_BITS) - 1);

    // Get values for multiplication sequencer
    ms_size = get_ms_size (macroinstr->read());
    ms_src = get_ms_src (macroinstr->read());
    ms_src_n = get_ms_src_n (macroinstr->read());
    ms_dst = get_ms_dst (macroinstr->read());
    ms_dst_n = get_ms_dst_n (macroinstr->read());
    csdrf_rd_addr->write(get_ms_csd_addr (macroinstr->read()));

    // Manage autoincrement of ITT index
    if ((decode_en && !nop_active) || decoding_reg) {    // If starting or continuing to decode macroinstruction
        decoding_nxt = true;

        // If new decode, start autoincrementing from macroinstruction itt_idx. Also, register common fields
        if (decode_en && !nop_active) {
            itt_idx_nxt = macro_itt_idx + 1;
            itt_idx_temp = macro_itt_idx;
            count_en = true;    // Also trigger increment of PC to advance macroinstruction

            common_temp = macroinstr->read() & ((1UL << SHIFT_ITT_IDX) - 1);

        // If continuing decoding, keep incrementing itt_idx
        } else {
            itt_idx_nxt = itt_idx_reg + 1;
        }

        // If stop, stop decoding and next itt_idx is SAFE_STATE
        itt_aux = ittm_field;
        if (itt_aux.stop) {
            itt_idx_nxt = uint(MACRO_IDX::SAFE_STATE);
            decoding_nxt = false;
        }
    }
    itt_idx = itt_idx_temp;
    common_nxt = common_temp;

    // Unpack common_fields into the fields for each DLB
    itt_aux = ittm_field;
    mov_valid = itt_aux.valid;
    itt_aux = ittsa_field;
    sa_valid = itt_aux.valid;
    itt_aux = ittpm_field;
    pm_valid = itt_aux.valid;
    unpack_fields (common_temp, mov_valid, sa_valid, pm_valid,
                   &mov_fields_aux, &sa_fields_aux, &pm_fields_aux);
    mov_fields = mov_fields_aux;
    sa_fields = sa_fields_aux;
    pm_fields = pm_fields_aux;

    // Trigger activation of multiplication sequencer
    ms_en = false;
    if (itt_idx_temp == uint(MACRO_IDX::VFUX_MUL)) {
        ms_en = true;
    }

    // Obtain the DLB indices from the ITT fields and the multiplication sequencer
    // Also, generate decode enable for the DLBs, from ITT first field + NOP control
    itt_aux = ittm_field;
    dlbm_index = itt_aux.addr;
    idm_decode_en = ms_mov_valid || (itt_aux.valid && !nop_active);
    itt_aux = ittsa_field;
    dlbsa_index = ms_sa_valid ? ms_sa_index : itt_aux.addr;
    idsa_decode_en = ms_sa_valid || (itt_aux.valid && !nop_active);
    itt_aux = ittpm_field;
    dlbpm_index = itt_aux.addr;
    idpm_decode_en = itt_aux.valid && !nop_active;

    // Manage S&A SRC0 if coming from VWR
    if (idsa_sa_op1_from == MUX::VWR) {
        sa_op1_from->write(idm_sa_op1_from);
    } else {
        sa_op1_from->write(idsa_sa_op1_from);
    }
}

void control_unit::output_method() {
    pc_out->write(pc);

    // Coalesce control signals from the different instruction decoders
    for (uint i = 0; i < REG_NUM; i++) {
        reg_en[i]->write(idm_reg_en[i] | idsa_reg_en[i] | idpm_reg_en[i]);
        reg_from[i]->write(idm_reg_from[i] | idsa_reg_from[i] | idpm_reg_from[i]);
    }
}

#if EN_MODEL
void control_unit::energy_thread() {

    EN_OP curOp;
    SWSIZE curSwSize;
    ADDOP curAddOp;
    uint curShift;
    SWREPACK curRepack;
    CP_EN_OP curCpInst;

    while (1) {
        // Default values
        curOp = EN_OP::IDLE;
        curSwSize = SWSIZE::INV;
        curAddOp = ADDOP::NOP;
        curShift = 0;
        curRepack = SWREPACK::INV;
        curCpInst = CP_EN_OP::IDLE;

        if (sa_en) {
            if (is_vwr(sa_op1_from)) {
                curOp = EN_OP::VWR_R1;
            } else if (sa_op1_from == MUX::R3 || sa_op2_from == MUX::ZERO) {
                curOp = EN_OP::R4_R1;
            }
            // Track SWSIZE
            curSwSize = sa_size;    
            // Track ADDOP
            if (sa_adder_en) {
                if (!sa_neg_op1 && !sa_neg_op2) {
                    curAddOp = ADDOP::ADD;
                } else if (!sa_neg_op1 && sa_neg_op2) {
                    curAddOp = ADDOP::SUB;
                } else if (sa_neg_op1 && !sa_neg_op2) {
                    curAddOp = ADDOP::INV;
                }
            }
            // Track SA_SHIFT
            curShift = sa_shift;
        }
        // Track SWREPACK
        curRepack = pm_repack;

        // Track CP instruction
        if (ms_sa_valid || ms->state_reg != MS_FSM::IDLE) {
            curCpInst = CP_EN_OP::MUL;
        } else if (ib_wr_en) {
            curCpInst = CP_EN_OP::WRF_IB;
        } else if (csdrf_wr_en) {
            curCpInst = CP_EN_OP::WRF_CSD;
        } else if (ms_csd_len_en || loop_reg_en) {
            curCpInst = CP_EN_OP::WRF_CSDLEN_LOOP;
        } else if (nop_active || (decode_en && itt_idx == uint(MACRO_IDX::NOP))) {  // It might not coincide with the number of NOPs as there are MULs
            curCpInst = CP_EN_OP::NOP;
        } else if ((decode_en && itt_idx == uint(MACRO_IDX::RLB)) || (idm_decode_en && itt_idx == (uint(MACRO_IDX::RLB)+1))) {
            curCpInst = CP_EN_OP::RLB;
        } else if (decode_en && itt_idx == uint(MACRO_IDX::WLB)) {
            curCpInst = CP_EN_OP::WLB;
        } else if (decode_en && itt_idx == uint(MACRO_IDX::VMV)) {
            curCpInst = CP_EN_OP::VMV;
        } else if (decode_en && (itt_idx == uint(MACRO_IDX::VFUX_SHIFT_FROMVWR) || itt_idx == uint(MACRO_IDX::VFUX_SHIFT_FROMR3)
                                || itt_idx == uint(MACRO_IDX::VFUX_SHIFT_FROMTOVWR))) {
            curCpInst = CP_EN_OP::SHIFT;
        } else if (decode_en && (itt_idx == uint(MACRO_IDX::VFUX_ADD_FROMVWR) || itt_idx == uint(MACRO_IDX::VFUX_ADD_FROMR3)
                                || itt_idx == uint(MACRO_IDX::VFUX_ADD_FROMTOVWR))) {
            curCpInst = CP_EN_OP::ADD;
        } else if (decode_en && (itt_idx == uint(MACRO_IDX::PACK_TOVWR) || itt_idx == uint(MACRO_IDX::PACK_TOR3))) {
            curCpInst = CP_EN_OP::PACK;
        } else if (decode_en && (itt_idx == uint(MACRO_IDX::EXIT))) {
            curCpInst = CP_EN_OP::EXIT;
        }

        // Update stats
        stats_s1[uint(curOp)][uint(curSwSize)][uint(curAddOp)][curShift]++;
        stats_s2[uint(curRepack)]++;
        stats_cp[uint(curCpInst)]++;

        wait();
    }
}

// Write energy stats to file
void control_unit::write_energy_stats() {    
    
    // Open file for recording stats for energy model
    std::string fo = "INPUTS_DIR/recording/" + filename + "_stage1.csv";
    ef_s1.open(fo);
    fo = "INPUTS_DIR/recording/" + filename + "_stage2.csv";
    ef_s2.open(fo);
    fo = "INPUTS_DIR/recording/" + filename + "_cp.csv";
    ef_cp.open(fo);
    if (!ef_s1.is_open() || !ef_s2.is_open() || !ef_cp.is_open()) {
        std::cerr << "Error opening files for energy recording" << std::endl;
        return;
    }
    
    // Write stage 1 stats
    ef_s1 << "Operation,Size,AddOp,Shift,Count" << std::endl;
    for (uint i = 0; i < uint(EN_OP::MAX); i++) {
        for (uint j = 0; j < uint(SWSIZE::B24)+1; j++) {
            for (uint k = 0; k < uint(ADDOP::MAX); k++) {
                for (uint l = 0; l < SA_MAX_SHIFT+1; l++) {
                    ef_s1 << EN_OP_STRING.at(EN_OP(i)) << ",";
                    ef_s1 << swsize_to_uint(SWSIZE(j)) << ",";
                    ef_s1 << SAOP_STRING.at(ADDOP(k)) << ",";
                    ef_s1 << l << ",";
                    ef_s1 << stats_s1[i][j][k][l] << std::endl;
                }
            }
        }
    }

    // Write stage 2 stats, iterating over all SWREPACK values (SWREPACK_LIST)
    ef_s2 << "Repack,Count" << std::endl;
    uint possibleRepacks = std::extent<decltype(SWREPACK_LIST)>::value;
    for (uint i = 0; i < possibleRepacks; i++) {
        ef_s2 << REPACK_SEL_STRING.at(SWREPACK_LIST[i]) << ",";
        ef_s2 << stats_s2[uint(SWREPACK_LIST[i])] << std::endl;
    }

    // Write CP stats
    ef_cp << "Instruction,Count" << std::endl;
    for (uint i = 0; i < uint(CP_EN_OP::MAX); i++) {
        ef_cp << CP_EN_OP_STRING.at(CP_EN_OP(i)) << ",";
        ef_cp << stats_cp[i] << std::endl;
    }

    // Close files
    ef_s1.close();
    ef_s2.close();
    ef_cp.close();
}
#endif // EN_MODEL
