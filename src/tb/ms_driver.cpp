#include "ms_driver.h"

void ms_driver::driver_thread() {

    uint clk_period = 10;

    // Initialization
    rst->write(false);
    enable->write(false);
    dst->write(OPC_STORAGE::R3);
    csd_in[0]->write(0);
    csd_len->write(CSD_BITS / 2);
    wait(clk_period / 2, SC_NS);
    rst->write(true);
    wait(0, SC_NS);

    wait(clk_period, SC_NS);

    cout << "Zero multiplication ----------------" << endl;
    enable->write(true);
    dst->write(OPC_STORAGE::R3);
    csd_in[0]->write(0);
    wait(clk_period, SC_NS);

    enable->write(false);
    wait(15*clk_period, SC_NS);

    enable->write(true);
    dst->write(OPC_STORAGE::R1R2);
    csd_in[0]->write(0);
    wait(clk_period, SC_NS);

    enable->write(false);
    wait(15*clk_period, SC_NS);

    cout << "Multiply by 7 ----------------" << endl;
    enable->write(true);
    dst->write(OPC_STORAGE::R3);
    csd_in[0]->write(0b01000010);
    wait(clk_period, SC_NS);

    enable->write(false);
    wait(15*clk_period, SC_NS);

    enable->write(true);
    dst->write(OPC_STORAGE::R1R2);
    csd_in[0]->write(0b01000010);
    wait(clk_period, SC_NS);

    enable->write(false);
    wait(15*clk_period, SC_NS);

    cout << "Multiply by 2**23 ----------------" << endl;
    enable->write(true);
    dst->write(OPC_STORAGE::R3);
    csd_in[0]->write(0x400000000000);
    wait(clk_period, SC_NS);

    enable->write(false);
    wait(15*clk_period, SC_NS);

    enable->write(true);
    dst->write(OPC_STORAGE::R1R2);
    csd_in[0]->write(0x400000000000);
    wait(clk_period, SC_NS);

    enable->write(false);
    wait(15*clk_period, SC_NS);

    cout << "Multiply by 100 ----------------" << endl;
    enable->write(true);
    dst->write(OPC_STORAGE::R3);
    csd_in[0]->write(0b0100100000010000);
    wait(clk_period, SC_NS);

    enable->write(false);
    wait(15*clk_period, SC_NS);

    enable->write(true);
    dst->write(OPC_STORAGE::R1R2);
    csd_in[0]->write(0b0100100000010000);
    wait(clk_period, SC_NS);

    enable->write(false);
    wait(15*clk_period, SC_NS);

    cout << "Multiply by 1963 ----------------" << endl;
    enable->write(true);
    dst->write(OPC_STORAGE::R3);
    csd_in[0]->write(0b010000000010001000100010);
    wait(clk_period, SC_NS);

    enable->write(false);
    wait(15*clk_period, SC_NS);

    enable->write(true);
    dst->write(OPC_STORAGE::R1R2);
    csd_in[0]->write(0b010000000010001000100010);
    wait(clk_period, SC_NS);

    enable->write(false);
    wait(15*clk_period, SC_NS);


//    for (i=0;; i++) {
//    }
}
