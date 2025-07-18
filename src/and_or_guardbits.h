/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Set of AND and OR gates to set the proper value to guardbits
 *
 */

#ifndef SRC_AND_OR_GUARDBITS_H_
#define SRC_AND_OR_GUARDBITS_H_

#include "systemc.h"

#include "cnm_base.h"

class and_or_guardbits: public sc_module {
public:
    sc_in<bool>         gb_to_one;          // Signals if performing an addition (0) or a subtraction (1)
    sc_in<uint>         size;               // Size of subwords
    sc_in<uint64_t>     input[WORD_64B];    // Input
    sc_out<uint64_t>    output[WORD_64B];   // Output

    // Internal signals and variables

    SC_CTOR(and_or_guardbits) {

        SC_METHOD(comb_method);
        sensitive << gb_to_one << size;
        for (int i = 0; i < WORD_64B; i++)
            sensitive << input[i];

    }

    // Mask with AND / OR gates
    void comb_method() {

        uint i;
        uint64_t            out_temp[WORD_64B];
        sc_bv<64>           parse_aux;
        sc_bv<WORD_64B*64>  in_aux, out_aux, mask_aux;
        sc_bv<WORD_64B*64>  zeros('0');
        sc_bv<WORD_64B*64>  ones('1');

        // Parse input to SystemC types
        for (i = 0; i < WORD_64B; i++) {
            parse_aux = input[i];
            in_aux.range((i+1)*64-1, i*64) = parse_aux;
        }

        // If size is not invalid
        if (size->read()) {
#ifndef __SYNTHESIS__
            if (gb_to_one->read()) {
                // Create and OR mask for subtracting
                mask_aux = zeros;
                for (i = 0; i < WORD_BITS/size; i++) {
                    mask_aux[(i+1)*size-1] = '1';
                }
                out_aux = in_aux | mask_aux;
            } else {
                // Create and AND mask for adding
                mask_aux = ones;
                for (i = 0; i < WORD_BITS/size; i++) {
                    mask_aux[(i+1)*size-1] = '0';
                }
                out_aux = in_aux & mask_aux;
            }
#else
            mask_guardbits_switch(in_aux, out_aux, size->read(), gb_to_one->read());
#endif
        } else {
            out_aux = in_aux;
        }

        // Parse SystemC result to output
        for (i = 0; i < WORD_64B; i++) {
            parse_aux.range(63,0) = out_aux.range((i+1)*64-1, i*64);
            out_temp[i] = parse_aux.to_uint64();
        }

        // Write to output
        for (i = 0; i < WORD_64B; i++) {
            output[i]->write(out_temp[i]);
        }

    }

#ifdef __SYNTHESIS__

    template<uint size>
    void mask_guardbits(sc_bv<WORD_64B*64> input, sc_bv<WORD_64B*64> &output, bool gb_to_one) {
        uint n_subwords = WORD_BITS/size;
        
        output = input;
        for (uint i = 0; i < n_subwords; i++) {
            output[(i+1)*size-1] = gb_to_one ? '1' : '0';
        }
    }

    void mask_guardbits_switch(sc_bv<WORD_64B*64> input, sc_bv<WORD_64B*64> &output, uint size, bool gb_to_one) {
        switch (size) {
            case 3:     mask_guardbits<3>(input, output, gb_to_one);     break;
            case 4:     mask_guardbits<4>(input, output, gb_to_one);     break;
            case 6:     mask_guardbits<6>(input, output, gb_to_one);     break;
            case 8:     mask_guardbits<8>(input, output, gb_to_one);     break;
            case 12:    mask_guardbits<12>(input, output, gb_to_one);    break;
            case 16:    mask_guardbits<16>(input, output, gb_to_one);    break;
            case 24:    mask_guardbits<24>(input, output, gb_to_one);    break;
            default:    output = input;                             break;
        }
    }
#endif
};

#endif /* SRC_AND_OR_GUARDBITS_H_ */
