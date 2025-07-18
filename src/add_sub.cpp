/*
 * Copyright EPFL 2021
 * Rafael Medina Morillas
 *
 * Implementation of an Adder/Subtractor.
 *
 */

#include "add_sub.h"

void add_sub::comb_method() {
    uint i;
    uint64_t        out_temp;
    sc_biguint<65>  op1_aux, op2_aux, res_aux;
    sc_uint<64>     parse_aux;
    sc_uint<1>      carry = neg_op1 || neg_op2;

    // Use 65 bits to continue carry chain of 64-bit addition
    for (i = 0; i < WORD_64B; i++) {
        op1_aux = neg_op1 ? ~op1[i] : op1[i];
        op2_aux = neg_op2 ? ~op2[i] : op2[i];
        res_aux = op1_aux + op2_aux + carry;
        carry = res_aux.range(64,64);
        parse_aux = res_aux(63,0);
        out_temp = parse_aux.to_uint64();
        output[i]->write(out_temp);
    }
}
