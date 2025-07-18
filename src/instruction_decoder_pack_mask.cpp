/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Implementation of the Instruction Decoder for Pack & Mask instructions.
 *
 */

#include "instruction_decoder_pack_mask.h"

void instruction_decoder_pack_mask::comb_method() {
    uint16_t microinstr = instr->read();
    uint32_t fields = common_fields->read();

    MASKOP maskop;
    OPC_STORAGE dst, src0, src1;
    SWREPACK repack;
    uint pack_sta, shift;
    bool perm;

    // ** DEFAULT VALUES FOR SIGNALS **

    // Ports
    for (uint i = 0; i < REG_NUM; i++) {
        reg_en[i]->write(false);
        reg_from[i]->write(MUX::EXT);
    }

    mrf_rd_addr->write(0);

    pm_en->write(false);
    pm_repack->write(SWREPACK::INV);
    pm_in_start->write(0);
    pm_op_sel->write(MASKOP::NOP);
    pm_shift->write(0);
    pm_op1_from->write(MUX::R1);
    pm_op2_from->write(MUX::R2);
    pm_out_to_vwr->write(false);

    // Intermediate variables

    // Decode new instruction when signaled (if any)
    if (decode_en->read()) {

        decode_pm_microinstr(microinstr, fields, &maskop, &dst, &perm, &src0, &src1, &repack, &pack_sta, &shift);  // Decode the microinstruction + fields from macro

        // Enable Pack & Mask Stage
        pm_en->write(true);

        // Control for source and destination storage
        switch (src0) {  // Read from source
            case OPC_STORAGE::R1:
                pm_op1_from->write(MUX::R1);
            break;
            default:
            break;
        }

        switch (src1) {  // Read from source
            case OPC_STORAGE::R2:
                pm_op2_from->write(MUX::R2);
            break;
            default:
            break;
        }

        switch (dst) {  // Store in destination
            case OPC_STORAGE::R3:
                reg_en[3]->write(true);
                reg_from[3]->write(MUX::PM);
            break;
            case OPC_STORAGE::VWR:
                pm_out_to_vwr->write(true);
            break;
            default:
            break;
        }

        // Set Mask address
//        mrf_rd_addr->write();

        // Generate data packer control
        pm_repack->write(repack);
        if (perm && repack != SWREPACK::INV) {
            pm_in_start->write(pack_sta + WORD_BITS/swsize_to_uint((repack_to_insize(repack))));
        } else {
            pm_in_start->write(pack_sta);
        }

        // Set Mask operation
        pm_op_sel->write(maskop);

        // Shift control
        pm_shift->write(shift);

    }
}
