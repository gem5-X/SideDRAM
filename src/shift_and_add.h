/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the SoftSIMD Shift & Add stage
 *
 */

#ifndef SRC_SHIFT_AND_ADD_H_
#define SRC_SHIFT_AND_ADD_H_

#include "systemc.h"

#include "add_sub.h"
#include "right_shifter.h"
#include "and_or_guardbits.h"
#include "adder_msb_set.h"
#include "cnm_base.h"

class shift_and_add: public sc_module {
public:

    sc_in_clk           clk;
    sc_in<bool>         rst;
    sc_in<bool>         compute_en;         // Signals that the stage should compute
    sc_in<uint>         shift;              // Positions to shift right
    sc_in<SWSIZE>       size;               // Size of subwords
    sc_in<bool>         adder_en;           // Selects if bypassing the adder
    sc_in<bool>         neg_op1;            // Signals if subtracting op1
    sc_in<bool>         neg_op2;            // Signals if subtracting op2
    sc_in<uint64_t>     op1[WORD_64B];      // First operand (to the shifter)
    sc_in<uint64_t>     op2[WORD_64B];      // Second operand
    sc_out<uint64_t>    output[WORD_64B];   // Output of the addition

    // Internal signals
    sc_signal<uint64_t>     shift_out[WORD_64B], add_out[WORD_64B]; // Output of the shifter and adder/subtractor
#if STAGE1_CYCLES
    sc_signal<uint64_t>     to_pipeline[WORD_64B], pipeline[WORD_64B][SA_CYCLES];   // Pipelined shift&add results
#endif
#if SA_AND_OR
    sc_signal<uint64_t>     and_or_out1[WORD_64B], and_or_out2[WORD_64B], msb_set_out[WORD_64B];
#endif
    sc_signal<uint>         decoded_size;
    sc_signal<bool>         gb_to_one1, gb_to_one2;

    // Submodules
    right_shifter<SA_MAX_SHIFT> *shifter;
#if SA_AND_OR
    and_or_guardbits *aogb1;
    and_or_guardbits *aogb2;
    adder_msb_set *ams;
#endif
    add_sub *add;

    SC_HAS_PROCESS(shift_and_add);
    shift_and_add(sc_module_name name) : sc_module(name) {
        uint i;

        shifter = new right_shifter<SA_MAX_SHIFT>("shifter");
        for (i = 0; i < WORD_64B; i++)
            shifter->input[i](op1[i]);
        shifter->shift(shift);
        shifter->size(decoded_size);
        for (i = 0; i < WORD_64B; i++)
            shifter->output[i](shift_out[i]);

#if SA_AND_OR
        aogb1 = new and_or_guardbits("and_or_guardbits_op1");
        aogb1->gb_to_one(gb_to_one1);
        aogb1->size(decoded_size);
        for (i = 0; i < WORD_64B; i++) {
            aogb1->input[i](shift_out[i]);
            aogb1->output[i](and_or_out1[i]);
        }

        aogb2 = new and_or_guardbits("and_or_guardbits_op2");
        aogb2->gb_to_one(gb_to_one2);
        aogb2->size(decoded_size);
        for (i = 0; i < WORD_64B; i++) {
            aogb2->input[i](op2[i]);
            aogb2->output[i](and_or_out2[i]);
        }

        add = new add_sub("adder_subtractor");
        add->neg_op1(neg_op1);
        add->neg_op2(neg_op2);
        for (i = 0; i < WORD_64B; i++) {
            add->op1[i](and_or_out1[i]);
            add->op2[i](and_or_out2[i]);
            add->output[i](add_out[i]);
        }

        ams = new adder_msb_set("adder_msb_set");
        ams->size(decoded_size);
        ams->neg_op1(neg_op1);
        ams->neg_op2(neg_op2);
        for (i = 0; i < WORD_64B; i++) {
            ams->op1[i](shift_out[i]);
            ams->op2[i](op2[i]);
            ams->add_out[i](add_out[i]);
            ams->output[i](msb_set_out[i]);
        }
#else
        add = new add_sub("adder_subtractor");
        add->neg_op1(neg_op1);
        add->neg_op2(neg_op2);
        for (i = 0; i < WORD_64B; i++) {
            add->op1[i](shift_out[i]);
            add->op2[i](op2[i]);
            add->output[i](add_out[i]);
        }
#endif

#if STAGE1_CYCLES
        uint j;
        for (i = 0; i < WORD_64B; i++)
            for (j = 0; j < SA_CYCLES; j++)
                pipeline[i][j] = 0;

        SC_THREAD(clk_thread);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);
#endif

        SC_METHOD(comb_method);
        sensitive << compute_en << adder_en << size << neg_op1 << neg_op2;
        for (i = 0; i < WORD_64B; i++) {
            sensitive << add_out[i] << shift_out[i];
#if SA_AND_OR
            sensitive << msb_set_out[i];
#endif
#if STAGE1_CYCLES
            sensitive << pipeline[i][SA_CYCLES - 1];
#endif
        }
    }

    void clk_thread();  // Advances the pipeline
    void comb_method(); // Connects the last pipeline register with the output
};

#endif /* SRC_SHIFT_AND_ADD_H_ */
