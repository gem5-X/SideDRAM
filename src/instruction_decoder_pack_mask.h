/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the Instruction Decoder for Pack & Mask instructions.
 *
 */

#ifndef SRC_INSTRUCTION_DECODER_PACK_MASK_H_
#define SRC_INSTRUCTION_DECODER_PACK_MASK_H_

#include "systemc.h"

#include "cnm_base.h"

class instruction_decoder_pack_mask: public sc_module {
public:
    sc_in_clk           clk;
    sc_in<bool>         rst;
    sc_in<bool>         decode_en;      // Enables decoding of the next instruction
    sc_in<uint16_t>     instr;          // Instruction input from DLB
    sc_in<uint32_t>     common_fields;  // Common fields from macroinstruction buffer

    // Registers control
    sc_out<bool>        reg_en[REG_NUM];    // Write enable for the registers in the datapath
    sc_out<MUX>         reg_from[REG_NUM];  // Index the MUX for input data

    // Mask RF control
    sc_out<uint>        mrf_rd_addr;    // Index read

    // Pack and Mask control
    sc_out<bool>        pm_en;          // Enables pack & mask stage
    sc_out<SWREPACK>    pm_repack;      // Repack of the subwords
    sc_out<uint>        pm_in_start;    // Position of subword in w1+w2 where packing to output starts
    sc_out<MASKOP>      pm_op_sel;      // Selection of mask operation
    sc_out<uint>        pm_shift;       // Positions to shift right
    sc_out<MUX>         pm_op1_from;    // Index MUX for input data 1
    sc_out<MUX>         pm_op2_from;    // Index MUX for input data 2
    sc_out<bool>        pm_out_to_vwr;  // Specifies that PM output goes to the active VWR in MOV


    // ** INTERNAL SIGNALS AND VARIABLES **

    SC_CTOR(instruction_decoder_pack_mask) {

        SC_METHOD(comb_method);
        sensitive << instr << common_fields << decode_en;

    }

    void comb_method(); // Performs the combinational logic
};

#endif /* SRC_INSTRUCTION_DECODER_PACK_MASK_H_ */
