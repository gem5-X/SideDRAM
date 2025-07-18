/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Implementation of the SoftSIMD mask unit
 *
 */

#include "mask_unit.h"

void mask_unit::comb_method() {
    uint i;
    sc_bv<MASK_64B*64>  mask_aux;       // Holds parsed mask
    sc_bv<MASK_BITS>    mask_to_rep;    // Holds mask before replication
    sc_bv<WORD_64B*64>  mask_rep('0');  // Holds replicated mask in SystemC bit-vector
    sc_bv<64>           parse_aux;      // Used for parsing
    uint64_t            mask[WORD_64B]; // Replicated mask in 64-bit uint
    uint64_t            out_temp = 0;

    // Parse mask input to SystemC types
    for (i = 0; i < MASK_64B; i++) {
        parse_aux = mask_in[i];
        mask_aux.range((i+1)*64-1, i*64) = parse_aux;
    }
    mask_to_rep = mask_aux(MASK_BITS-1,0);

    // Replicate mask
    for (i = 0; i < MASK_PER_WORD; i++) {
        mask_rep.range((i+1)*MASK_BITS-1,i*MASK_BITS) = mask_to_rep;
    }

    // Parse replicated mask to 64-bit uint
    for (i = 0; i < WORD_64B; i++) {
        parse_aux.range(63,0) = mask_rep.range((i+1)*64-1, i*64);
        mask[i] = parse_aux.to_uint64();
    }

    for (i = 0; i < WORD_64B; i++) {
        switch (op_sel->read()) {
            case MASKOP::NOP:
                out_temp = word_in[i]->read();
            break;
            case MASKOP::AND:
                out_temp = word_in[i]->read() & mask[i];
            break;
            case MASKOP::OR:
                out_temp = word_in[i]->read() | mask[i];
            break;
            case MASKOP::XOR:
                out_temp = word_in[i]->read() ^ mask[i];
            break;
            default:
                out_temp = word_in[i]->read();
            break;
        }
        output[i]->write(out_temp);
    }
}
