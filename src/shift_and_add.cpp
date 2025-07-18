/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Implementation of the SoftSIMD Shift & Add stage
 *
 */

#include "shift_and_add.h"

#if STAGE1_CYCLES
void shift_and_add::clk_thread() {
    int i, j;

    // Reset behaviour
    for (i = 0; i < WORD_64B; i++)
        for (j = 0; j < SA_CYCLES; j++)
            pipeline[i][j] = 0;

    wait();

    // Clocked behaviour
    while (1) {
        if (compute_en->read()) {
            for (i = 0; i < WORD_64B; i++) {
                for (j = SA_CYCLES - 1; j > 0; j--)
                    pipeline[i][j] = pipeline[i][j - 1];

                pipeline[i][0] = to_pipeline[i];
            }
        }
        wait();
    }
}
#endif

void shift_and_add::comb_method() {

#if STAGE1_CYCLES
    if (compute_en->read()) {
        for (int i = 0; i < WORD_64B; i++) {
            if (adder_en->read())
                to_pipeline[i] = add_out[i];
            else
                to_pipeline[i] = shift_out[i];
        }
    } else {
        for (int i = 0; i < WORD_64B; i++) {
            to_pipeline[i] = 0;
        }
    }

    for (uint i = 0; i < WORD_64B; i++) {
        output[i]->write(pipeline[i][SA_CYCLES - 1]);
    }
#else
    if (compute_en->read()) {
        for (int i = 0; i < WORD_64B; i++) {
            if (adder_en->read()) {
#if SA_AND_OR
                output[i] = msb_set_out[i];
#else
                output[i] = add_out[i];
#endif
            } else {
                output[i] = shift_out[i];
            }
        }
    } else {
        for (int i = 0; i < WORD_64B; i++) {
            output[i] = 0;
        }
    }

#endif

    decoded_size = swsize_to_uint(size->read());
    gb_to_one1 = !neg_op1 && neg_op2;
    gb_to_one2 = neg_op1 && !neg_op2;
}
