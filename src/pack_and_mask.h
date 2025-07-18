/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the SoftSIMD Pack & Mask stage
 *
 */

#ifndef SRC_PACK_AND_MASK_H_
#define SRC_PACK_AND_MASK_H_

#include "systemc.h"

#include "cnm_base.h"
#include "data_pack.h"
#include "mask_unit.h"
#include "right_shifter.h"

class pack_and_mask: public sc_module {
public:

    sc_in_clk           clk;
    sc_in<bool>         rst;
    sc_in<bool>         compute_en;
    // Data packing
    sc_in<uint64_t>     w1[WORD_64B];       // First word input
    sc_in<uint64_t>     w2[WORD_64B];       // Second word input
    sc_in<SWREPACK>     repack;             // Repack control
    sc_in<uint>         in_start;           // Position of subword in w1+w2 where packing to output starts
    // Masking
    sc_in<uint64_t>     mask_in[MASK_64B];  // Input mask
    sc_in<MASKOP>       op_sel;             // Selection of mask operation
    // Shifter
    sc_in<uint>         shift;              // Positions to shift right
    sc_out<uint64_t>    output[WORD_64B];   // Output of the stage

    // Internal signals
    sc_signal<uint64_t> pack_out[WORD_64B], mask_out[WORD_64B], shift_out[WORD_64B]; // Outputs of the submodules
#if STAGE2_CYCLES
    sc_signal<uint64_t> to_pipeline[WORD_64B], pipeline[WORD_64B][PM_CYCLES];    // Pipelined pack&mask results
#endif
    sc_signal<uint>     decoded_in_size, decoded_out_size;

    // Submodules
    data_pack *dp;
    mask_unit *mu;
    right_shifter<PM_MAX_SHIFT> *shifter;

    SC_HAS_PROCESS(pack_and_mask);
    pack_and_mask(sc_module_name name) : sc_module(name) {
        uint i;

        dp = new data_pack("data_packer");
        for (i = 0; i < WORD_64B; i++)
            dp->w1[i](w1[i]);
        for (i = 0; i < WORD_64B; i++)
            dp->w2[i](w2[i]);
        dp->in_size(decoded_in_size);
        dp->out_size(decoded_out_size);
        dp->in_start(in_start);
        for (i = 0; i < WORD_64B; i++)
            dp->output[i](pack_out[i]);

        mu = new mask_unit("mask_unit");
        for (i = 0; i < WORD_64B; i++)
            mu->word_in[i](pack_out[i]);
        for (i = 0; i < MASK_64B; i++)
            mu->mask_in[i](mask_in[i]);
        mu->op_sel(op_sel);
        for (i = 0; i < WORD_64B; i++)
            mu->output[i](mask_out[i]);

        shifter = new right_shifter<PM_MAX_SHIFT>("shifter");
        for (i = 0; i < WORD_64B; i++)
            shifter->input[i](mask_out[i]);
        shifter->shift(shift);
        shifter->size(decoded_out_size);
        for (i = 0; i < WORD_64B; i++)
            shifter->output[i](shift_out[i]);

#if STAGE2_CYCLES
        uint j;
        for (i = 0; i < WORD_64B; i++)
            for (j = 0; j < PM_CYCLES; j++)
                pipeline[i][j] = 0;

        SC_THREAD(clk_thread);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);
#endif

        SC_METHOD(comb_method);
        sensitive << compute_en << repack;
        for (i = 0; i < WORD_64B; i++) {
            sensitive << shift_out[i];
#if STAGE2_CYCLES
            sensitive << pipeline[i][PM_CYCLES - 1];
#endif
        }
    }

    void clk_thread();  // Advances the pipeline
    void comb_method(); // Connects the last pipeline register with the output

};

#endif /* SRC_PACK_AND_MASK_H_ */
