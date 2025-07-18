/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the three Index Translation Tables as a ROMs (Read-Only Memories)
 *
 */

#include "cnm_base.h"

#ifndef SRC_ITT_ROM_H_
#define SRC_ITT_ROM_H_

class itt_mov: public sc_module {
public:
    sc_in<uint>         index;          // Index from the macroinstruction buffer to be translated
    sc_out<itt_field>   translation;    // Translation output (valid microinstruction, address and stop)

    SC_CTOR(itt_mov) {
        SC_METHOD(read_method);
        sensitive << index;
    }

    // Performs the index translation
    void read_method() {
        translation->write(itt_mov_contents[index]);
    }
};

class itt_sa: public sc_module {
public:
    sc_in<uint>         index;          // Index from the macroinstruction buffer to be translated
    sc_out<itt_field>   translation;    // Translation output (valid microinstruction, address and stop)

    SC_CTOR(itt_sa) {
        SC_METHOD(read_method);
        sensitive << index;
    }

    // Performs the index translation
    void read_method() {
        translation->write(itt_sa_contents[index]);
    }
};


class itt_pm: public sc_module {
public:
    sc_in<uint>         index;          // Index from the macroinstruction buffer to be translated
    sc_out<itt_field>   translation;    // Translation output (valid microinstruction, address and stop)

    SC_CTOR(itt_pm) {
        SC_METHOD(read_method);
        sensitive << index;
    }

    // Performs the index translation
    void read_method() {
        translation->write(itt_pm_contents[index]);
    }
};


#endif /* SRC_ITT_ROM_H_ */
