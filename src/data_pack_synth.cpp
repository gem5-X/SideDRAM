/*
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 *
 * Implementation of the SoftSIMD data packer for synthesis
 *
 */

#include "systemc.h"
#include "cnm_base.h"

#ifdef __SYNTHESIS__

#include "data_pack.h"

void data_pack::circular_shift_right(sc_bv<2*WORD_BITS> &w1_w2, uint n_bits) {
    uint i;
    sc_bv<2*WORD_BITS> w1_w2_aux, or_aux;

    if (n_bits) {
        w1_w2_aux = w1_w2;
        w1_w2_aux <<= (2*WORD_BITS - n_bits);
        or_aux = w1_w2 >> n_bits;
        w1_w2_aux |= or_aux;
    }
    w1_w2 = w1_w2_aux;
}

// NOTE only 8 values for in_start allowed
void data_pack::circ_shift_preshuffle(sc_bv<2*WORD_BITS> &w1_w2, uint in_size, uint out_size, uint in_start) {
    uint idx;
    sc_bv<2*WORD_BITS> w1w2_aux = w1_w2;

    switch (in_size) {
        case 3:
            switch (out_size) {
                case 4:
                    idx = (in_start / 4) * 12;  // Shift at 4 words granularity
                    circular_shift_right(w1w2_aux, idx);
                break;
                default:    break;
            }
        break;
        case 4:
            switch (out_size) {
                case 3:
                    idx = (in_start / 4) * 16;  // Shift at 4 words granularity
                    circular_shift_right(w1w2_aux, idx);
                break;
                case 6:
                    idx = (in_start / 4) * 16;  // Shift at 4 words granularity
                    circular_shift_right(w1w2_aux, idx);
                break;
                default:    break;
            }
        break;
        case 6:
            switch (out_size) {
                case 4:
                    idx = (in_start / 4) * 24;  // Shift at 4 words granularity
                    circular_shift_right(w1w2_aux, idx);
                break;
                case 8:
                    idx = (in_start / 2) * 12;  // Shift at 2 words granularity
                    circular_shift_right(w1w2_aux, idx);
                break;
                default:    break;
            }
        break;
        case 8:
            switch (out_size) {
                case 6:
                    idx = (in_start / 2) * 16;  // Shift at 2 words granularity
                    circular_shift_right(w1w2_aux, idx);
                break;
                case 12:
                    idx = (in_start / 2) * 16;  // Shift at 2 words granularity
                    circular_shift_right(w1w2_aux, idx);
                break;
                default:    break;
            }
        break;
        case 12:
            switch (out_size) {
                case 8:
                    idx = (in_start / 2) * 24;  // Shift at 2 words granularity
                    circular_shift_right(w1w2_aux, idx);
                break;
                case 16:
                    idx = in_start * 12;        // Shift at 1 word granularity
                    circular_shift_right(w1w2_aux, idx);
                break;
                default:    break;
            }
        break;
        case 16:
            switch (out_size) {
                case 12:
                    idx = in_start * 16;        // Shift at 1 word granularity
                    circular_shift_right(w1w2_aux, idx);
                break;
                case 24:
                    idx = in_start * 16;        // Shift at 1 word granularity
                    circular_shift_right(w1w2_aux, idx);
                break;
                default:    break;
            }
        break;
        case 24:
            switch (out_size) {
                case 16:
                    idx = in_start * 24;        // Shift at 1 word granularity
                    circular_shift_right(w1w2_aux, idx);
                break;
                default:    break;
            }
        break;
        default:    break;
    }
    w1_w2 = w1w2_aux;
}

void data_pack::shuffle_subwords(sc_bv<WORD_64B*64> w1_aux, sc_bv<WORD_64B*64> w2_aux, sc_bv<WORD_64B*64> &output,
                      uint in_size, uint out_size, uint in_start) {
    uint i;
    sc_bv<2*WORD_BITS> w1_w2;

    // Pack w1 and w2 into a single word and perform circular shift
    w1_w2.range(WORD_BITS-1, 0) = w1_aux.range(WORD_BITS-1, 0);
    w1_w2.range(2*WORD_BITS-1, WORD_BITS) = w2_aux.range(WORD_BITS-1, 0);
    circ_shift_preshuffle(w1_w2, in_size, out_size, in_start);

    switch (in_size) {
        case 3:     
            switch (out_size) {
                case 4:
                    for (i = 0; i < WORD_BITS/4; i++)
                        output.range((i+1)*4-1, i*4+1) = w1_w2.range((i+1)*3-1, i*3);
                break;
                default:    output = w1_aux;    break;
            }
        break;
        case 4:     
            switch (out_size) {
                case 3:
                    for (i = 0; i < WORD_BITS/3; i++)
                        output.range((i+1)*3-1, i*3) = w1_w2.range((i+1)*4-1, i*4+1);
                break;
                case 6:
                    for (i = 0; i < WORD_BITS/6; i++)
                        output.range((i+1)*6-1, i*6+2) = w1_w2.range((i+1)*4-1, i*4);
                break;
                default:    output = w1_aux;    break;
            }
        break;
        case 6:     
            switch (out_size) {
                case 4:
                    for (i = 0; i < WORD_BITS/4; i++)
                        output.range((i+1)*4-1, i*4) = w1_w2.range((i+1)*6-1, i*6+2);
                break;
                case 8:
                    for (i = 0; i < WORD_BITS/8; i++)
                        output.range((i+1)*8-1, i*8+2) = w1_w2.range((i+1)*6-1, i*6);
                break;
                default:    output = w1_aux;    break;
            }
        break;
        case 8:     
            switch (out_size) {
                case 6:
                    for (i = 0; i < WORD_BITS/6; i++)
                        output.range((i+1)*6-1, i*6) = w1_w2.range((i+1)*8-1, i*8+2);
                break;
                case 12:
                    for (i = 0; i < WORD_BITS/12; i++)
                        output.range((i+1)*12-1, i*12+4) = w1_w2.range((i+1)*8-1, i*8);
                break;
                default:    output = w1_aux;    break;
            }
        break;
        case 12:    
            switch (out_size) {
                case 8:
                    for (i = 0; i < WORD_BITS/8; i++)
                        output.range((i+1)*8-1, i*8) = w1_w2.range((i+1)*12-1, i*12+4);
                break;
                case 16:
                    for (i = 0; i < WORD_BITS/16; i++)
                        output.range((i+1)*16-1, i*16+4) = w1_w2.range((i+1)*12-1, i*12);
                break;
                default:    output = w1_aux;    break;
            }
        break;
        case 16:    
            switch (out_size) {
                case 12:
                    for (i = 0; i < WORD_BITS/12; i++)
                        output.range((i+1)*12-1, i*12) = w1_w2.range((i+1)*16-1, i*16+4);
                break;
                case 24:
                    for (i = 0; i < WORD_BITS/24; i++)
                        output.range((i+1)*24-1, i*24+8) = w1_w2.range((i+1)*16-1, i*16);
                break;
                default:    output = w1_aux;    break;
            }
        break;
        case 24:    
            switch (out_size) {
                case 16:
                    for (i = 0; i < WORD_BITS/16; i++)
                        output.range((i+1)*16-1, i*16) = w1_w2.range((i+1)*24-1, i*24+8);
                break;
                default:    output = w1_aux;    break;
            }
        break;
        default:    output = w1_aux;    break;
    }
}

#endif  // __SYNTHESIS__
