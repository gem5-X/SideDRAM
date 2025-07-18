#include "ms_driver.h"
#include "ms_monitor.h"
#include "../mult_sequencer.h"
#include <string>

int sc_main(int argc, char *argv[]) {
    sc_clock                clk("clk", 10, SC_NS);
    sc_signal<bool>         rst;
    sc_signal<bool>         enable;             // Signals if macroinstruction buffer is pointing here
    sc_signal<OPC_STORAGE>  dst;                // DST specified by macroinstruction
    sc_signal<uint64_t>     csd_in[CSD_64B];    // CSD multiplier input
    sc_signal<uint8_t>      csd_len;            // Length of CSD multiplier input
    sc_signal<bool>         sa_valid;           // Signals if S&A index is valid
    sc_signal<uint>         sa_index;           // Index for the S&A DLB microcode
    sc_signal<OPC_STORAGE>  sa_src0;            // Sets SRC0 for the current microinstruction
    sc_signal<OPC_STORAGE>  sa_dst;             // Sets DST for the current microinstruction
    sc_signal<uint>         dst_uint;
    sc_signal<uint>         sa_src0_uint;
    sc_signal<uint>         sa_dst_uint;


    mult_sequencer dut("MultSequencerUnderTest");
    dut.clk(clk);
    dut.rst(rst);
    dut.enable(enable);
    dut.dst(dst);
    for (uint i = 0; i < CSD_64B; i++)
        dut.csd_in[i](csd_in[i]);
    dut.csd_len(csd_len);
    dut.sa_valid(sa_valid);
    dut.sa_index(sa_index);
    dut.sa_src0(sa_src0);
    dut.sa_dst(sa_dst);

    ms_driver driver("Driver");
    driver.rst(rst);
    driver.enable(enable);
    driver.dst(dst);
    for (uint i = 0; i < CSD_64B; i++)
        driver.csd_in[i](csd_in[i]);
    driver.csd_len(csd_len);

    ms_monitor monitor("Monitor");
    monitor.clk(clk);
    monitor.rst(rst);
    monitor.enable(enable);
    monitor.dst(dst);
    for (uint i = 0; i < CSD_64B; i++)
        monitor.csd_in[i](csd_in[i]);
    monitor.csd_len(csd_len);
    monitor.sa_valid(sa_valid);
    monitor.sa_index(sa_index);
    monitor.sa_src0(sa_src0);
    monitor.sa_dst(sa_dst);
    monitor.dst_uint(dst_uint);
    monitor.sa_src0_uint(sa_src0_uint);
    monitor.sa_dst_uint(sa_dst_uint);

    sc_trace_file *tracefile;
    tracefile = sc_create_vcd_trace_file("waveforms/ms_wave");

    sc_trace(tracefile, clk, "clk");
    sc_trace(tracefile, rst, "rst");
//    sc_trace(tracefile, dut.state_test, "state");
//    sc_trace(tracefile, dut.window_test, "window");
//    sc_trace(tracefile, dut.idx_reg, "window_index");
    sc_trace(tracefile, enable, "enable");
    sc_trace(tracefile, dst_uint, "dst");
    sc_trace(tracefile, csd_in[0], "csd_in");
    sc_trace(tracefile, csd_len, "csd_len");
    sc_trace(tracefile, sa_valid, "sa_valid");
    sc_trace(tracefile, sa_index, "sa_index");
    sc_trace(tracefile, sa_src0_uint, "sa_src0");
    sc_trace(tracefile, sa_dst_uint, "sa_dst");

    sc_start(2000, SC_NS);

    sc_close_vcd_trace_file(tracefile);

    return 0;
}
