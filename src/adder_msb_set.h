/*
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 *
 * Description of the XOR gates for the subword guard-bits/MSBs after addition/subtraction
 *
 */

#ifndef SRC_ADDER_MSB_SET_H_
#define SRC_ADDER_MSB_SET_H_

#include "systemc.h"
#include "cnm_base.h"

class adder_msb_set: public sc_module {
public:
    sc_in<uint>         size;               // Subword size
    sc_in<bool>         neg_op1;            // Signals if subtracting op1
    sc_in<bool>         neg_op2;            // Signals if subtracting op2
    sc_in<uint64_t>     op1[WORD_64B];      // First operand before setting the guardbit
    sc_in<uint64_t>     op2[WORD_64B];      // Second operand before setting the guardbit
    sc_in<uint64_t>     add_out[WORD_64B];  // Adder/subtractor output
    sc_out<uint64_t>    output[WORD_64B];   // Output of the MSB setting

    // Internal signals and variables

    SC_CTOR(adder_msb_set) {

        SC_METHOD(comb_method);
        sensitive << size << neg_op1 << neg_op2;
        for (uint i = 0; i < WORD_64B; i++)
            sensitive << op1[i] << op2[i] << add_out[i];
    }

    // Set the correct subword MSBs after addition/subtraction
    void comb_method() {
        uint i;
        sc_bv<64>           parse_aux;  // Used for parsing
        sc_bv<WORD_64B*64>  op1_aux;    // Holds casted op1
        sc_bv<WORD_64B*64>  op2_aux;    // Holds casted op2
        sc_bv<WORD_64B*64>  ao_aux;     // Holds casted add_out
        sc_bv<WORD_64B*64>  out_aux;    // Holds casted output
        uint64_t            temp;

        if (size) {
            // Parse inputs to SystemC types, already accounting for the negation in case of subtraction
            for (i = 0; i < WORD_64B; i++) {
                temp = neg_op1 ? ~op1[i] : op1[i];
                parse_aux = temp;
                op1_aux.range((i+1)*64-1, i*64) = parse_aux;
                temp = neg_op2 ? ~op2[i] : op2[i];
                parse_aux = temp;
                op2_aux.range((i+1)*64-1, i*64) = parse_aux;
                temp = add_out[i];
                parse_aux = temp;
                ao_aux.range((i+1)*64-1, i*64) = parse_aux;
            }

#ifndef __SYNTHESIS__
            out_aux = ao_aux;

            // Set the correct MSB for each subword
            for (i = 0; i < WORD_BITS/size; i++) {
                uint gb_idx = (i+1)*size-1;
                out_aux[gb_idx] = op1_aux[gb_idx] ^ op2_aux[gb_idx] ^ ao_aux[gb_idx];
            }
#else
            set_msb_switch(op1_aux, op2_aux, ao_aux, out_aux, size);
#endif

            // Parse SystemC result to output
            for (i = 0; i < WORD_64B; i++) {
                parse_aux.range(63,0) = out_aux.range((i+1)*64-1, i*64);
                temp = parse_aux.to_uint64();
                output[i]->write(temp);
            }

        } else {
            for (i = 0; i < WORD_64B; i++)
                output[i]->write(add_out[i]);
        }

    }

#ifdef __SYNTHESIS__
    template<uint size>
    void set_msb(sc_bv<WORD_64B*64> &op1, sc_bv<WORD_64B*64> &op2, sc_bv<WORD_64B*64> &ao, sc_bv<WORD_64B*64> out) {
        uint n_subwords = WORD_BITS/size;

        out = ao;
        for (uint i = 0; i < n_subwords; i++) {
            uint gb_idx = (i+1)*size-1;
            out[gb_idx] = op1[gb_idx] ^ op2[gb_idx] ^ ao[gb_idx];
        }
    }

    void set_msb_switch(sc_bv<WORD_64B*64> &op1, sc_bv<WORD_64B*64> &op2, sc_bv<WORD_64B*64> &ao, sc_bv<WORD_64B*64> out, uint size) {
        switch (size) {
            case 3:     set_msb<3>(op1, op2, ao, out);  break;
            case 4:     set_msb<4>(op1, op2, ao, out);  break;
            case 6:     set_msb<6>(op1, op2, ao, out);  break;
            case 8:     set_msb<8>(op1, op2, ao, out);  break;
            case 12:    set_msb<12>(op1, op2, ao, out); break;
            case 16:    set_msb<16>(op1, op2, ao, out); break;
            case 24:    set_msb<24>(op1, op2, ao, out); break;
            default:    out = ao;                       break;
        }
    }
#endif
};


#endif /* SRC_ADDER_MSB_SET_H_ */
