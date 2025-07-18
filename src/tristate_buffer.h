/*
 * Copyright EPFL 2022
 * Rafael Medina Morillas
 *
 * Description of a generic tri-state buffer.
 *
 */

#ifndef TRISTATE_BUFFER_H_
#define TRISTATE_BUFFER_H_

#ifndef __SYNTHESIS__

#include "systemc.h"

template<uint width>
class tristate_buffer: public sc_module {
public:
    sc_in<sc_lv<width> >    input;
    sc_in<bool>             enable;
    sc_out<sc_lv<width> >   output;

    SC_CTOR(tristate_buffer) {
        SC_METHOD(comb_method);
        sensitive << input << enable;
    }

    // Controls the buffer
    void comb_method() {
        sc_lv<width> allzs(SC_LOGIC_Z);

        if (enable->read())
            output->write(input->read());
        else
            output->write(allzs);
    }
};

#endif

#endif /* TRISTATE_BUFFER_H_ */
