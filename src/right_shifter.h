/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of a right shifter
 *
 */

#ifndef SRC_RIGHT_SHIFTER_H_
#define SRC_RIGHT_SHIFTER_H_

#include "systemc.h"

#include "cnm_base.h"

template<uint max_shift>
class right_shifter: public sc_module {
public:
    sc_in<uint64_t>     input[WORD_64B];    // Input
    sc_in<uint>         shift;              // Positions to shift right
    sc_in<uint>         size;               // Size of subwords
    sc_out<uint64_t>    output[WORD_64B];   // Output

    // Internal signals and variables

    SC_CTOR(right_shifter) {

        SC_METHOD(comb_method);
        sensitive << shift << size;
        for (uint i = 0; i < WORD_64B; i++)
            sensitive << input[i];
    }

    // Shift input if below maximum shift
    void comb_method() {

        uint i, j;
        uint64_t            out_temp[WORD_64B]; // Holds output while shifting
        sc_bv<64>           parse_aux;          // Used for parsing
        sc_bv<WORD_64B*64>  in_aux;             // Holds casted input
        sc_bv<WORD_64B*64>  out_aux('0');       // Holds casted output

        // If shift is zero or too large, bypass shifter
        if (shift > max_shift || shift == 0) {
            for (i = 0; i < WORD_64B; i++)
                out_temp[i] = input[i];

        } else {

            // Parse input to SystemC types
            for (i = 0; i < WORD_64B; i++) {
                parse_aux = input[i];
                in_aux.range((i+1)*64-1, i*64) = parse_aux;
            }

            // Shift word
            out_aux = in_aux >> shift;

            // Sign extension if size > 0
            if (size) {
#ifndef __SYNTHESIS__
                for (i = 0; i < WORD_BITS/size; i++) {
                    // Keep guard bits
                    out_aux[(i+1)*size-1] = in_aux[(i+1)*size-1];

                    // Sign extend
                    for (j = 0; j < shift; j++)
                        if (int((i+1)*size-1-j) > int(i*size-1)) { // Control overflow to next subword
                            out_aux[(i+1)*size-1-j] = in_aux[(i+1)*size-1];
                        }
                }
#else   // __SYNTHESIS__
                guard_keep_sign_extend(in_aux, out_aux, size->read(), shift->read());
#endif  // __SYNTHESIS__
            }

            // Parse SystemC result to output
            for (i = 0; i < WORD_64B; i++) {
                parse_aux.range(63,0) = out_aux.range((i+1)*64-1, i*64);
                out_temp[i] = parse_aux.to_uint64();
            }
        }

        // Write to output
        for (i = 0; i < WORD_64B; i++) {
            output[i]->write(out_temp[i]);
        }
    }

#ifdef __SYNTHESIS__

    template <uint size>
    void sign_extend(sc_bv<WORD_64B*64> input, sc_bv<WORD_64B*64> &output, uint shift, uint sw) {
        switch (shift) {
            case 1: for (int j = 0; j < 1; j++) if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            case 2: for (int j = 0; j < 2; j++) if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            case 3: for (int j = 0; j < 3; j++) if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            case 4: for (int j = 0; j < 4; j++) if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            case 5: for (int j = 0; j < 5; j++) if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            case 6: for (int j = 0; j < 6; j++) if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            case 7: for (int j = 0; j < 7; j++) if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            case 8: for (int j = 0; j < 8; j++) if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            case 9: for (int j = 0; j < 9; j++) if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            case 10:    for (int j = 0; j < 10; j++)    if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            case 11:    for (int j = 0; j < 11; j++)    if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            case 12:    for (int j = 0; j < 12; j++)    if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            case 13:    for (int j = 0; j < 13; j++)    if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            case 14:    for (int j = 0; j < 14; j++)    if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            case 15:    for (int j = 0; j < 15; j++)    if (int((sw+1)*size-2-j) > int(sw*size-1))  output[(sw+1)*size-2-j] = input[(sw+1)*size-2]; break;
            default:    break;
        }
    }

    void guard_keep_sign_extend(sc_bv<WORD_64B*64> input, sc_bv<WORD_64B*64> &output, uint size, uint shift) {
        switch (size) {
            case 3:
                for (int i = 0; i < WORD_BITS/3; i++) {
                    output[(i+1)*3-1] = input[(i+1)*3-1];
                    sign_extend<3>(input, output, shift, i);
                }
            break;
            case 4:
                for (int i = 0; i < WORD_BITS/4; i++) {
                    output[(i+1)*4-1] = input[(i+1)*4-1];
                    sign_extend<4>(input, output, shift, i);
                }
            break;
            case 6:
                for (int i = 0; i < WORD_BITS/6; i++) {
                    output[(i+1)*6-1] = input[(i+1)*6-1];
                    sign_extend<6>(input, output, shift, i);
                }
            break;
            case 8:
                for (int i = 0; i < WORD_BITS/8; i++) {
                    output[(i+1)*8-1] = input[(i+1)*8-1];
                    sign_extend<8>(input, output, shift, i);
                }
            break;
            case 12:
                for (int i = 0; i < WORD_BITS/12; i++) {
                    output[(i+1)*12-1] = input[(i+1)*12-1];
                    sign_extend<12>(input, output, shift, i);
                }
            break;
            case 16:
                for (int i = 0; i < WORD_BITS/16; i++) {
                    output[(i+1)*16-1] = input[(i+1)*16-1];
                    sign_extend<16>(input, output, shift, i);
                }
            break;
            case 24:
                for (int i = 0; i < WORD_BITS/24; i++) {
                    output[(i+1)*24-1] = input[(i+1)*24-1];
                    sign_extend<24>(input, output, shift, i);
                }
            break;
            default:    break;
        }
    }
#endif
};

#endif /* SRC_RIGHT_SHIFTER_H_ */
