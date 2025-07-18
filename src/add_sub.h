/*
 * Copyright EPFL 2021
 * Rafael Medina Morillas
 *
 * Description of an Adder/Subtractor.
 *
 */

#ifndef SRC_ADD_SUB_H_
#define SRC_ADD_SUB_H_

#include "systemc.h"

#include "cnm_base.h"

class add_sub: public sc_module {
public:
    sc_in<bool>         neg_op1;            // Signals if subtracting op1
    sc_in<bool>         neg_op2;            // Signals if subtracting op2
    sc_in<uint64_t>     op1[WORD_64B];      // First operand
    sc_in<uint64_t>     op2[WORD_64B];      // Second operand
    sc_out<uint64_t>    output[WORD_64B];   // Output of the addition/subtraction

    SC_CTOR(add_sub) {
        uint i;

        SC_METHOD(comb_method);
        sensitive << neg_op1 << neg_op2;
        for (i = 0; i < WORD_64B; i++)
            sensitive << op1[i] << op2[i];
    }

    void comb_method();   // Performs casting and the addition/substraction
};

#endif /* SRC_ADD_SUB_H_ */
