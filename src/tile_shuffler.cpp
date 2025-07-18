/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Implementation of the SoftSIMD data packer
 *
 */

#include "tile_shuffler.h"

void tile_shuffler::clk_thread() {
    // Reset behavior
    shuffle_reg = 0;

    wait();

    // Clocked behavior
    while (1) {
        if (input_en) {
            shuffle_reg = shuffle_nxt;
        }
        wait();
    }
}

#ifndef __SYNTHESIS__

void tile_shuffler::comb_method() {

    uint intervals;
    sc_lv<WORD_BITS> broadcast;
    sc_lv<VWR_BITS> repeat;
    sc_lv<VWR_BITS> shift_r('0'), shift_l('0');

    TS_MODE mode_aux = mode;
    mode_test = (uint) mode_aux;

    shuffle_nxt = shuffle_reg;

    if (input_en->read() && out_start->read() < WORDS_PER_VWR) {

        // Get only the relevant bits from the vww bus
        vww_aux = vww_inout;
        in_aux = 0;
        in_aux.range(WORDS_PER_VWR*WORD_BITS-1, 0) = vww_aux.range(WORDS_PER_VWR*WORD_BITS-1, 0);

        shuffle_aux = in_aux;

        switch (mode) {

            case TS_MODE::NOP:  // Input = output
            break;
            case TS_MODE::SHIFT:    // Circular shift right <out_start> words the VWR
                shift_r = in_aux >> (out_start*WORD_BITS);
                shift_l = in_aux << ((WORDS_PER_VWR - out_start)*WORD_BITS);
                shuffle_aux = shift_r | shift_l;
            break;
            case TS_MODE::BROADCAST:    // Broadcast <out_start> word to fill the VWR
                broadcast = in_aux.range((out_start+1)*WORD_BITS-1, out_start*WORD_BITS);
                for (uint i = 0; i < WORDS_PER_VWR; i++) {
                    shuffle_aux.range((i+1)*WORD_BITS-1, i*WORD_BITS) = broadcast;
                }
            break;
            case TS_MODE::REPEAT:   // Repeat each word <out_start> times until filling the VWR
                intervals = WORDS_PER_VWR / out_start;
                for (uint i = 0; i < intervals; i++) {
                    repeat = in_aux.range((i+1)*WORD_BITS-1, i*WORD_BITS);
                    for (uint j = 0; j < out_start; j++) {
                        shuffle_aux.range(i*out_start*WORD_BITS+(j+1)*WORD_BITS-1, i*out_start*WORD_BITS+j*WORD_BITS) = repeat;
                    }
                }
            break;
            default:    // Input = output
            break;
        }

        // Write out through the tristate buffer
        shuffle_nxt = shuffle_aux;
    }
}

#else   // __SYNTHESIS__

void tile_shuffler::comb_method() {

    uint intervals;
    sc_lv<WORD_BITS> broadcast;
    sc_lv<WORD_BITS> repeat;
    sc_lv<VWR_BITS> shift_r('0'), shift_l('0');
    sc_lv<VWR_BITS> shift_aux;

    // Make up for the lack of tri-state buffers
//    vww_out = vww_in; // This creates combinational loop
    vww_out->write(0);

    TS_MODE mode_aux = mode;
    mode_test = (uint) mode_aux;

    shuffle_nxt = shuffle_reg;

    if (input_en->read() && out_start->read() < WORDS_PER_VWR) {

        in_aux = vww_in;
        shuffle_aux = in_aux;

        switch (mode) {

            case TS_MODE::NOP:  // Input = output
            break;
            case TS_MODE::SHIFT:    // Circular shift right <out_start> words the VWR
                shift_aux = in_aux;
                shift_aux >>= out_start*WORD_BITS;
                shift_r = shift_aux;
                // shift_r = in_aux >> (out_start*WORD_BITS);
                shift_aux = in_aux;
                shift_aux <<= (WORDS_PER_VWR - out_start)*WORD_BITS;
                shift_l = shift_aux;
                // shift_l = in_aux << ((WORDS_PER_VWR - out_start)*WORD_BITS);
                shuffle_aux = shift_r | shift_l;
            break;
            case TS_MODE::BROADCAST:    // Broadcast <out_start> word to fill the VWR
                get_word_switch(in_aux, broadcast, out_start);
                for (uint i = 0; i < WORDS_PER_VWR; i++) {
                    // shift_aux.range((i+1)*WORD_BITS-1, i*WORD_BITS) = broadcast;
                    put_word_switch(broadcast, shuffle_aux, i);
                }
            break;
            case TS_MODE::REPEAT:   // Repeat each word <out_start> times until filling the VWR
                repeat_words_switch(in_aux, shuffle_aux, out_start);
            break;
            default:    // Input = output
            break;
        }

        // Write out through the tristate buffer
        shuffle_nxt = shuffle_aux;
    }

    // Make up for lack of tri-state buffers
    if (output_en->read()) {
        vww_out->write(shuffle_reg);
    }
}

void tile_shuffler::repeat_words_switch(sc_lv<VWR_BITS> input, sc_lv<VWR_BITS> &output, uint out_start) {
    switch (out_start) {
        case 2:     repeat_words<2>(input, output);     break;
        case 3:     repeat_words<3>(input, output);     break;
        case 4:     repeat_words<4>(input, output);     break;
        case 5:     repeat_words<5>(input, output);     break;
        case 6:     repeat_words<6>(input, output);     break;
        case 7:     repeat_words<7>(input, output);     break;
        case 8:     repeat_words<8>(input, output);     break;
        case 9:     repeat_words<9>(input, output);     break;
        case 10:    repeat_words<10>(input, output);    break;
        case 11:    repeat_words<11>(input, output);    break;
        case 12:    repeat_words<12>(input, output);    break;
        case 13:    repeat_words<13>(input, output);    break;
        case 14:    repeat_words<14>(input, output);    break;
        case 15:    repeat_words<15>(input, output);    break;
        case 16:    repeat_words<16>(input, output);    break;
        default:    break;
    }
}

template <uint out_start>
void tile_shuffler::repeat_words(sc_lv<VWR_BITS> input, sc_lv<VWR_BITS> &output) {
    sc_lv<WORD_BITS> repeat;
    uint intervals = WORDS_PER_VWR / out_start;   

    for (uint i = 0; i < intervals; i++) {
        get_word_switch(input, repeat, i);
        for (uint j = 0; j < out_start; j++) {
            put_word_switch(repeat, output, i*out_start+j);
        }
    }
}

void tile_shuffler::get_word_switch(sc_lv<VWR_BITS> input, sc_lv<WORD_BITS> &output, uint idx) {
    switch (idx) {
        case 0:     get_word<0>(input, output);     break;
        case 1:     get_word<1>(input, output);     break;
        case 2:     get_word<2>(input, output);     break;
        case 3:     get_word<3>(input, output);     break;
        case 4:     get_word<4>(input, output);     break;
        case 5:     get_word<5>(input, output);     break;
        case 6:     get_word<6>(input, output);     break;
        case 7:     get_word<7>(input, output);     break;
        case 8:     get_word<8>(input, output);     break;
        case 9:     get_word<9>(input, output);     break;
        case 10:    get_word<10>(input, output);    break;
        case 11:    get_word<11>(input, output);    break;
        case 12:    get_word<12>(input, output);    break;
        case 13:    get_word<13>(input, output);    break;
        case 14:    get_word<14>(input, output);    break;
        case 15:    get_word<15>(input, output);    break;
        case 16:    get_word<16>(input, output);    break;
        default:    break;
    }
}

template <uint idx>
void tile_shuffler::get_word(sc_lv<VWR_BITS> input, sc_lv<WORD_BITS> &output) {
    if (idx < WORDS_PER_VWR) 
        output = input.range((idx+1)*WORD_BITS-1, idx*WORD_BITS);
}

void tile_shuffler::put_word_switch(sc_lv<WORD_BITS> input, sc_lv<VWR_BITS> &output, uint idx) {
    switch (idx) {
        case 0:     put_word<0>(input, output);     break;
        case 1:     put_word<1>(input, output);     break;
        case 2:     put_word<2>(input, output);     break;
        case 3:     put_word<3>(input, output);     break;
        case 4:     put_word<4>(input, output);     break;
        case 5:     put_word<5>(input, output);     break;
        case 6:     put_word<6>(input, output);     break;
        case 7:     put_word<7>(input, output);     break;
        case 8:     put_word<8>(input, output);     break;
        case 9:     put_word<9>(input, output);     break;
        case 10:    put_word<10>(input, output);    break;
        case 11:    put_word<11>(input, output);    break;
        case 12:    put_word<12>(input, output);    break;
        case 13:    put_word<13>(input, output);    break;
        case 14:    put_word<14>(input, output);    break;
        case 15:    put_word<15>(input, output);    break;
        case 16:    put_word<16>(input, output);    break;
        default:    break;
    }
}

template <uint idx>
void tile_shuffler::put_word(sc_lv<WORD_BITS> input, sc_lv<VWR_BITS> &output) {
    if (idx < WORDS_PER_VWR) 
        output.range((idx+1)*WORD_BITS-1, idx*WORD_BITS) = input;
}

#endif  // __SYNTHESIS__
