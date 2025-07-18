/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the SoftSIMD tile shuffler
 *
 */


#ifndef SRC_TILE_SHUFFLER_H_
#define SRC_TILE_SHUFFLER_H_

#include "systemc.h"

#include "cnm_base.h"
#include "tristate_buffer.h"

class tile_shuffler: public sc_module {
public:

#ifndef __SYNTHESIS__

    sc_in_clk               clk;
    sc_in<bool>             rst;
    sc_in<bool>             input_en;   // Enables the input for shuffling words
    sc_in<bool>             output_en;  // Enables the output after shuffling words
    sc_in<uint>             out_start;  // Start location for the words shuffled out
    sc_in<TS_MODE>          mode;       // Selects shuffling mode
    sc_inout_rv<VWR_BITS>   vww_inout;  // I/O very wide word

    // Internal signals and variables
    sc_signal<sc_lv<VWR_BITS> > shuffle_reg, shuffle_nxt;
    sc_lv<VWR_BITS>             in_aux, out_aux, shuffle_aux, vww_aux;
    sc_signal<uint>             mode_test;

    // Internal modules
    tristate_buffer<VWR_BITS> *inout_buffer;

    SC_CTOR(tile_shuffler) {

        inout_buffer = new tristate_buffer<VWR_BITS>("Inout_buffer");
        inout_buffer->input(shuffle_reg);
        inout_buffer->enable(output_en);
        inout_buffer->output(vww_inout);

        SC_THREAD(clk_thread);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);

        SC_METHOD(comb_method);
        sensitive << input_en << out_start << mode << vww_inout;
    }

    void clk_thread();  // Registers the results of tile shuffling
    void comb_method(); // Performs removal or addition of guard bits according to the flags

#else   // __SYNTHESIS__

    sc_in_clk                   clk;
    sc_in<bool>                 rst;
    sc_in<bool>                 input_en;   // Enables the input for shuffling words
    sc_in<bool>                 output_en;  // Enables the output after shuffling words
    sc_in<uint>                 out_start;  // Start location for the words shuffled out
    sc_in<TS_MODE>              mode;       // Selects shuffling mode
    sc_in<sc_lv<VWR_BITS> >     vww_in;     // I/O very wide word
    sc_out<sc_lv<VWR_BITS> >    vww_out;    // I/O very wide word

    // Internal signals and variables
    sc_signal<sc_lv<VWR_BITS> > shuffle_reg, shuffle_nxt;
    sc_lv<VWR_BITS>             in_aux, out_aux, shuffle_aux;
    sc_signal<uint>             mode_test;

    // Internal modules

    SC_CTOR(tile_shuffler) {

        SC_THREAD(clk_thread);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);

        SC_METHOD(comb_method);
        sensitive << input_en << out_start << mode << vww_in << output_en << shuffle_reg;
    }

    void clk_thread();  // Registers the results of tile shuffling
    void comb_method(); // Performs removal or addition of guard bits according to the flags
    void repeat_words_switch(sc_lv<VWR_BITS> input, sc_lv<VWR_BITS> &output, uint out_start);
    template <uint out_start>
    void repeat_words(sc_lv<VWR_BITS> input, sc_lv<VWR_BITS> &output);
    void get_word_switch(sc_lv<VWR_BITS> input, sc_lv<WORD_BITS> &output, uint idx);
    template <uint idx>
    void get_word(sc_lv<VWR_BITS> input, sc_lv<WORD_BITS> &output);
    void put_word_switch(sc_lv<WORD_BITS> input, sc_lv<VWR_BITS> &output, uint idx);
    template <uint idx>
    void put_word(sc_lv<WORD_BITS> input, sc_lv<VWR_BITS> &output);

#endif // __SYNTHESIS__

};

#endif /* SRC_TILE_SHUFFLER_H_ */
