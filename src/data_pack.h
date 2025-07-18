/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the SoftSIMD data packer
 *
 */

#ifndef SRC_DATA_PACK_H_
#define SRC_DATA_PACK_H_

#include "systemc.h"
#include "cnm_base.h"

class data_pack: public sc_module {
public:
    sc_in<uint64_t>     w1[WORD_64B];       // First word
    sc_in<uint64_t>     w2[WORD_64B];       // Second word
    sc_in<uint>         in_size;            // Size of the input subwords
    sc_in<uint>         out_size;           // Size of the output subwords
    sc_in<uint>         in_start;           // Position of subword in w1+w2 where packing to output starts
    sc_out<uint64_t>    output[WORD_64B];   // Output of the packing

    // Internal signals and variables

    SC_CTOR(data_pack) {
        uint i;

        SC_METHOD(comb_method);
        sensitive << in_size << out_size << in_start;
        for (i = 0; i < WORD_64B; i++)
            sensitive << w1[i] << w2[i];
    }

    void comb_method();   // Performs data packing according to the flags

#ifdef __SYNTHESIS__
    void circular_shift_right(sc_bv<2*WORD_BITS> &w1_w2, uint n_bits);
    void circ_shift_preshuffle(sc_bv<2*WORD_BITS> &w1_w2, uint in_size, uint out_size, uint in_start);
    void shuffle_subwords(sc_bv<WORD_64B*64> w1_aux, sc_bv<WORD_64B*64> w2_aux, sc_bv<WORD_64B*64> &output,
            uint in_size, uint out_size, uint in_start);
#endif
};

#endif /* SRC_DATA_PACK_H_ */
