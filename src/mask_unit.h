/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the SoftSIMD mask unit
 *
 */

#ifndef SRC_MASK_UNIT_H_
#define SRC_MASK_UNIT_H_

#include "systemc.h"

#include "cnm_base.h"

class mask_unit: public sc_module {
public:
    sc_in<uint64_t>     word_in[WORD_64B];    // Input word
    sc_in<uint64_t>     mask_in[MASK_64B];    // Input mask
    sc_in<MASKOP>       op_sel;               // Selection of mask operation
    sc_out<uint64_t>    output[WORD_64B];     // Masked output

    // Internal signals and variables

    SC_CTOR(mask_unit) {
        uint i;

        SC_METHOD(comb_method);
        sensitive << op_sel;
        for (i = 0; i < WORD_64B; i++)
            sensitive << word_in[i];
        for (i = 0; i < MASK_64B; i++)
            sensitive << mask_in[i];
    }

    void comb_method();   // Performs word masking

};

#endif /* SRC_MASK_UNIT_H_ */
