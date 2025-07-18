/*
 * Copyright EPFL 2021
 * Rafael Medina Morillas
 *
 * Description of a generic register
 *
 */

#ifndef REGISTER_H_
#define REGISTER_H_

#include "systemc.h"

template<uint W64B>
class reg: public sc_module {
public:
    sc_in_clk           clk;
    sc_in<bool>         rst;
    sc_in<bool>         en;
    sc_in<uint64_t>     input[W64B];
    sc_out<uint64_t>    output[W64B];

    // Internal signals and variables
    sc_signal<uint64_t> registered[W64B];

    SC_CTOR(reg) {
        uint i;

        SC_THREAD(clk_thread);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);

        SC_METHOD(comb_method);
        for (i=0; i < W64B; i++)
            sensitive << registered[i];

        for (i=0; i < W64B; i++)
            registered[i] = 0;
    }

    // Registers
    void clk_thread() {
        uint i;

        // Reset behaviour
        for (i = 0; i < W64B; i++) {
            registered[i] = 0;
        }

        wait();

        // Clocked behaviour
        while (1) {
            if (en->read()) {
                for (i = 0; i < W64B; i++) {
                    registered[i] = input[i];
                }
            }

            wait();
        }
    }

    // Connects register with output
    void comb_method() {
        for (uint i = 0; i < W64B; i++) {
            output[i]->write(registered[i]);
        }
    }
};

#endif /* REGISTER_H_ */
