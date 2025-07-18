/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Implementation of the SoftSIMD Pack & Mask stage
 *
 */

#include "pack_and_mask.h"

#if STAGE2_CYCLES
void pack_and_mask::clk_thread() {
    int i, j;

    // Reset behaviour
    for (i = 0; i < WORD_64B; i++)
        for (j = 0; j < PM_CYCLES; j++)
            pipeline[i][j] = 0;

    wait();

    // Clocked behaviour
    while (1) {
        if (compute_en->read()) {
            for (i = 0; i < WORD_64B; i++) {
                for (j = PM_CYCLES - 1; j > 0; j--)
                    pipeline[i][j] = pipeline[i][j - 1];
                pipeline[i][0] = to_pipeline[i];
            }
        }
        wait();
    }
}
#endif

void pack_and_mask::comb_method() {

#if STAGE2_CYCLES
    if (compute_en->read()) {
        for (int i = 0; i < WORD_64B; i++) {
            to_pipeline[i] = shift_out[i];
        }
    } else {
        for (int i = 0; i < WORD_64B; i++) {
            to_pipeline[i] = 0;
        }
    }

    for (uint i = 0; i < WORD_64B; i++) {
        output[i]->write(pipeline[i][PM_CYCLES - 1]);
    }
#else
    if (compute_en->read()) {
        for (int i = 0; i < WORD_64B; i++) {
            output[i] = shift_out[i];
        }
    } else {
        for (int i = 0; i < WORD_64B; i++) {
            output[i] = 0;
        }
    }
#endif

    SWSIZE in_size = repack_to_insize(repack->read());
    decoded_in_size = swsize_to_uint(in_size);
    SWSIZE out_size = repack_to_outsize(repack->read());
    decoded_out_size = swsize_to_uint(out_size);
}

