#include "softsimd_pu_monitor.h"

void softsimd_pu_monitor::monitor_thread() {/*
    sc_uint<64> instruction;
    sc_uint<OPCODE_STA - OPCODE_END + 1> OPCODE;
    while (1) {
        wait(1500, SC_PS);

        cout << "At time " << sc_time_stamp() << endl;
        cout << "RD " << RD << " WR " << WR << " ACT " << ACT << " AB_mode "
                << AB_mode << " pim_mode " << pim_mode << endl;
        cout << " Bank " << bank_addr << " Row " << row_addr << " Column "
                << col_addr << endl;
        cout << "External data DQ " << std::hex << DQ << std::dec << endl;
#if DUAL_BANK_INTERFACE
        cout << "Even bus " << even_bus << endl
                << "Odd bus " << odd_bus << endl;
#else
        cout << "DRAM bus " << dram_bus << endl;
#endif

        wait();
    }*/
}
