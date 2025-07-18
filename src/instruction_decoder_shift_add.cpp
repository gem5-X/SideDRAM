/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Implementation of the Instruction Decoder for Shift & Add instructions.
 *
 */

#include "instruction_decoder_shift_add.h"

void instruction_decoder_shift_add::comb_method() {
    uint16_t microinstr = instr->read();
    uint32_t fields = common_fields->read();

    ADDOP addop;
    uint shift;
    SWSIZE sw_len;
    OPC_STORAGE dst, src0, src1;

    // ** DEFAULT VALUES FOR SIGNALS **

    // Ports
    for (uint i = 0; i < REG_NUM; i++) {
        reg_en[i]->write(false);
        reg_from[i]->write(MUX::EXT);
    }

    srf_rd_addr->write(0);

    sa_en->write(false);
    sa_shift->write(0);
    sa_size->write(SWSIZE::INV);
    sa_adder_en->write(false);
    sa_neg_op1->write(false);
    sa_neg_op2->write(false);
    sa_op1_from->write(MUX::R3);
    sa_op2_from->write(MUX::R0);

    // Intermediate variables

    // Decode new instruction when signaled (if any)
    if (decode_en->read()) {

        decode_sa_microinstr(microinstr, fields, &addop, &shift, &sw_len, &dst, &src0, &src1);  // Decode the microinstruction + fields from macro
        if (ms_valid->read()) {
            sw_len = ms_size->read();
#if (INSTR_FORMAT == BASE_FORMAT)
            shift = ms_shift->read();
#endif
            dst = ms_dst->read();
            src0 = ms_src0->read();
        }

        // Enable Shift & Add Stage
        sa_en->write(true);

        // Control for source and destination storage
        switch (src0) {  // Read from source
            case OPC_STORAGE::R3:
                sa_op1_from->write(MUX::R3);
            break;
            case OPC_STORAGE::VWR:
                sa_op1_from->write(MUX::VWR);
            break;
            default:
            break;
        }
        switch (src1) {  // Read from source
            case OPC_STORAGE::R0:
                sa_op2_from->write(MUX::R0);
            break;
            case OPC_STORAGE::ZERO:
                sa_op2_from->write(MUX::ZERO);
            break;
            default:
            break;
        }

        switch (dst) {  // Store in destination
            case OPC_STORAGE::R1:
                reg_en[1]->write(true);
                reg_from[1]->write(MUX::SA);
            break;
            case OPC_STORAGE::R2:
                reg_en[2]->write(true);
                reg_from[2]->write(MUX::SA);
            break;
            case OPC_STORAGE::R1R2:
                reg_en[1]->write(true);
                reg_from[1]->write(MUX::SA);
                reg_en[2]->write(true);
                reg_from[2]->write(MUX::SA);
            break;
            case OPC_STORAGE::R3:
                reg_en[3]->write(true);
                reg_from[3]->write(MUX::SA);
            break;
            case OPC_STORAGE::VWR:
                // MOV instruction decoder will take charge
            break;
            default:
            break;
        }

        // Shift control
        sa_shift->write(shift);
        sa_size->write(sw_len);

        // Adder-subtractor control
        switch (addop) {
            case ADDOP::NOP:
            break;
            case ADDOP::ADD:
                sa_adder_en->write(true);
                sa_neg_op1->write(false);
                sa_neg_op2->write(false);
            break;
            case ADDOP::SUB:
                sa_adder_en->write(true);
                sa_neg_op1->write(false);
                sa_neg_op2->write(true);
            break;
            case ADDOP::INV:
                sa_adder_en->write(true);
                sa_neg_op1->write(true);
                sa_neg_op2->write(false);
                sa_op2_from->write(MUX::ZERO);  // TODO can we take charge of this before?
            break;
            default:
            break;
        }
    }
}
