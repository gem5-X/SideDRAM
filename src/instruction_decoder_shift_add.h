/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the Instruction Decoder for Shift & Add instructions.
 *
 */

#ifndef SRC_INSTRUCTION_DECODER_SHIFT_ADD_H_
#define SRC_INSTRUCTION_DECODER_SHIFT_ADD_H_

#include "systemc.h"

#include "cnm_base.h"

class instruction_decoder_shift_add: public sc_module {
public:
    sc_in_clk           clk;
    sc_in<bool>         rst;
    sc_in<bool>         decode_en;      // Enables decoding of the next instruction
    sc_in<uint16_t>     instr;          // Instruction input from DLB
    sc_in<uint32_t>     common_fields;  // Common fields from macroinstruction buffer
    sc_in<bool>         ms_valid;       // Signal commands from multiplication sequencer are valid
    sc_in<SWSIZE>       ms_size;        // Subword size coming from multiplication sequencer
#if (INSTR_FORMAT == BASE_FORMAT)
    sc_in<uint>         ms_shift;       // Shift coming from multiplication sequencer
#endif
    sc_in<OPC_STORAGE>  ms_dst;         // DST coming from multiplication sequencer
    sc_in<OPC_STORAGE>  ms_src0;        // SRC0 coming from multiplication sequencer

    // Registers control
    sc_out<bool>    reg_en[REG_NUM];    // Write enable for the registers in the datapath
    sc_out<MUX>     reg_from[REG_NUM];  // Index the MUX for input data

    // SRF control
    sc_out<uint>    srf_rd_addr;    // Index read

    // Shift and Add control
    sc_out<bool>    sa_en;          // Enables shift & add stage
    sc_out<uint>    sa_shift;       // Specifies shift
    sc_out<SWSIZE>  sa_size;        // Specifies subword size
    sc_out<bool>    sa_adder_en;    // Enables adder/subtractor
    sc_out<bool>    sa_neg_op1;     // Signals if subtracting op1
    sc_out<bool>    sa_neg_op2;     // Signals if subtracting op2
    sc_out<MUX>     sa_op1_from;    // Index MUX for input data 1
    sc_out<MUX>     sa_op2_from;    // Index MUX for input data 2

    // ** INTERNAL SIGNALS AND VARIABLES **

    SC_CTOR(instruction_decoder_shift_add) {

        SC_METHOD(comb_method);
        sensitive << instr << common_fields << decode_en;
        sensitive << ms_valid << ms_size << ms_dst << ms_src0;
#if (INSTR_FORMAT == BASE_FORMAT)
        sensitive << ms_shift;
#endif
    }

    void comb_method(); // Performs the combinational logic
};

#endif /* SRC_INSTRUCTION_DECODER_SHIFT_ADD_H_ */
