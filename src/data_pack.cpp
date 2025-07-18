/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Implementation of the SoftSIMD data packer
 *
 */

#include "data_pack.h"

void data_pack::comb_method() {
    uint        i;
    uint        in_idx;                 // Holds current input subword index
    bool        nw1_w2;                 // Selects if current subword index is in w1 (false) or in w2 (true)
    uint64_t    out_temp[WORD_64B];     // Holds output while packing
    sc_bv<WORD_64B*64>  w1_aux, w2_aux; // Hold casted w1 and w2
    sc_bv<WORD_64B*64>  out_aux('0');   // Holds casted output
    sc_bit              sign;           // Holds sign for sign extension
    sc_bv<64>           parse_aux;      // Used for parsing

    // Check if the packing is "legal" (otherwise, output same as w1)
    // Do we do this or do we assume control always generate good packings?
    if (in_size > MASK_BITS || out_size > MASK_BITS || in_size == 0 || out_size == 0) {
        for (i = 0; i < WORD_64B; i++)
            out_temp[i] = w1[i];
    // Check that the start position is valid
    } else if (in_start >= 2*(MASK_BITS/in_size)) {
        for (i = 0; i < WORD_64B; i++)
            out_temp[i] = w1[i];
    } else {

        // Parse input to SystemC types
        for (i = 0; i < WORD_64B; i++) {
            parse_aux = w1[i];
            w1_aux.range((i+1)*64-1, i*64) = parse_aux;
            parse_aux = w2[i];
            w2_aux.range((i+1)*64-1, i*64) = parse_aux;
        }

        // Shuffle bits
#if !LOCAL_REPACK   // Shuffling along the whole word
#ifndef __SYNTHESIS__
        for (i = 0; i < WORD_BITS/out_size; i++) {

            // Compute index of subword
            // Relative start index + MASK_PER_WORD offset, counting also overflow
            in_idx = (i + in_start) % (2*(WORD_BITS/in_size));
            nw1_w2 = in_idx >= WORD_BITS/in_size;

            // Align if w2
            if (nw1_w2)
                in_idx -= WORD_BITS/in_size;

            // Move input subword into output subword, left aligned, without sign extension (shifter will be used for that)
            if (in_size < out_size) {
                if (nw1_w2) {
                    out_aux.range((i+1)*out_size-1, (i+1)*out_size-in_size) = w2_aux.range((in_idx+1)*in_size-1,in_idx*in_size);
                } else {
                    out_aux.range((i+1)*out_size-1, (i+1)*out_size-in_size) = w1_aux.range((in_idx+1)*in_size-1,in_idx*in_size);
                }
            } else { // in_size > out_size, right now keeping MSBs
                if (nw1_w2) {
                    out_aux.range((i+1)*out_size-1, i*out_size) = w2_aux.range((in_idx+1)*in_size-1, (in_idx+1)*in_size-out_size);
                } else {
                    out_aux.range((i+1)*out_size-1, i*out_size) = w1_aux.range((in_idx+1)*in_size-1, (in_idx+1)*in_size-out_size);
                }
            }
        }
#else   // __SYNTHESIS__
        // loop_shuffling_switch(w1_aux, w2_aux, out_aux, in_size, out_size, in_start);
        shuffle_subwords(w1_aux, w2_aux, out_aux, in_size, out_size, in_start);
#endif  // __SYNTHESIS__
#else   // Shuffling within MASK_BITS chunks
        for (i = 0; i < MASK_PER_WORD; i++) {
            for (uint j = 0; j < MASK_BITS/out_size; j++) {

                // Compute index of subword
                // Relative start index + MASK_PER_WORD offset, counting also overflow
                in_idx = (j + in_start) % (2*(MASK_BITS/in_size));
                nw1_w2 = in_idx >= MASK_BITS/in_size;

                // Align if w2
                if (nw1_w2)
                    in_idx -= MASK_BITS/in_size;

                // Move input subword into output subword, left aligned, without sign extension (shifter will be used for that)
                if (in_size < out_size) {
                    if (nw1_w2) {
                        out_aux.range(i*MASK_BITS+(j+1)*out_size-1, i*MASK_BITS+(j+1)*out_size-in_size) =
                                w2_aux.range(i*MASK_BITS+(in_idx+1)*in_size-1,i*MASK_BITS+in_idx*in_size);
                    } else {
                        out_aux.range(i*MASK_BITS+(j+1)*out_size-1, i*MASK_BITS+(j+1)*out_size-in_size) =
                                w1_aux.range(i*MASK_BITS+(in_idx+1)*in_size-1,i*MASK_BITS+in_idx*in_size);
                    }
                } else { // in_size > out_size, right now keeping MSBs
                    if (nw1_w2) {
                        out_aux.range(i*MASK_BITS+(j+1)*out_size-1, i*MASK_BITS+j*out_size) =
                                w2_aux.range(i*MASK_BITS+(in_idx+1)*in_size-1,i*MASK_BITS+(in_idx+1)*in_size-out_size);
                    } else {
                        out_aux.range(i*MASK_BITS+(j+1)*out_size-1, i*MASK_BITS+j*out_size) =
                                w1_aux.range(i*MASK_BITS+(in_idx+1)*in_size-1,i*MASK_BITS+(in_idx+1)*in_size-out_size);
                    }
                }
            }
        }
#endif  // LOCAL_REPACK

        // Parse SystemC result to output
        for (i = 0; i < WORD_64B; i++) {
            parse_aux.range(63,0) = out_aux.range((i+1)*64-1, i*64);
            out_temp[i] = parse_aux.to_uint64();
        }
    }

    // Write to output
    for (i = 0; i < WORD_64B; i++)
        output[i]->write(out_temp[i]);
}
