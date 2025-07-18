/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the three Distributed Loop Buffers holding microcode in ROMs (Read-Only Memories)
 *
 */

#include "cnm_base.h"

#ifndef SRC_DLB_ROM_H_
#define SRC_DLB_ROM_H_

class dlb_mov: public sc_module {
public:
    sc_in<uint>         index;      // Index of the microinstruction
    sc_out<uint16_t>    microinstr; // Microinstruction output

    SC_CTOR(dlb_mov) {
        SC_METHOD(read_method);
        sensitive << index;
    }

    // Performs the index translation
    void read_method() {
        microinstr->write(dlb_mov_contents[index]);
    }
};

class dlb_sa: public sc_module {
public:
    sc_in<uint>         index;      // Index of the microinstruction
    sc_out<uint16_t>    microinstr; // Microinstruction output

    SC_CTOR(dlb_sa) {
        SC_METHOD(read_method);
        sensitive << index;
    }

    // Performs the index translation
    void read_method() {
        microinstr->write(dlb_sa_contents[index]);
    }
};


class dlb_pm: public sc_module {
public:
    sc_in<uint>         index;      // Index of the microinstruction
    sc_out<uint16_t>    microinstr; // Microinstruction output

    SC_CTOR(dlb_pm) {
        SC_METHOD(read_method);
        sensitive << index;
    }

    // Performs the index translation
    void read_method() {
        microinstr->write(dlb_pm_contents[index]);
    }
};


#endif /* SRC_DLB_ROM_H_ */
