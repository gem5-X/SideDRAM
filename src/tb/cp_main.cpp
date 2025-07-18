#include "cp_main.h"

#ifdef MTI_SYSTEMC

    SC_MODULE_EXPORT(top);

#ifdef MIXED_SIM

    void top::clock_method() {
        clk = static_cast<sc_logic>(clk_g);
    }

#endif // MIXED_SIM

#endif // MTI_SYSTEMC