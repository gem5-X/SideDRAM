#include "vwr_driver.h"
#include "vwr_monitor.h"
#include "../vwr.h"
#include <string>

int sc_main(int argc, char *argv[]) {
    sc_clock                        clk("clk", 10, SC_NS);
    sc_signal<bool>                 rst;
    // VWR
    sc_signal<bool>                 wr_nrd;
//    sc_signal_rv<<DRAM_BITS>        vwr_io;
    // MUX/DEMUX
    sc_signal<uint>                 idx;
//    sc_signal<uint>                 pos;
//    sc_signal<uint>                 len;
    sc_signal<bool>                 d_nm;
    sc_signal_rv<VWR_BITS>          lbus;
    sc_signal_rv<WORD_BITS>         sbus;

    vwr<VWR_BITS> vut("VWRUnderTest");
    vut.clk(clk);
    vut.rst(rst);
    vut.wr_nrd(wr_nrd);
    vut.port(lbus);

    mux_demux<VWR_BITS, WORD_BITS> mdut("MuxDemuxUnderTest");
    mdut.idx(idx);
    mdut.d_nm(d_nm);
//    mdut.pos(pos);
//    mdut.len(len);
    mdut.lbus(lbus);
    mdut.sbus(sbus);

    vwr_driver driver("Driver");
    driver.rst(rst);
    driver.wr_nrd(wr_nrd);
    driver.idx(idx);
    driver.d_nm(d_nm);
//    driver.pos(pos);
//    driver.len(len);
    driver.lbus(lbus);
    driver.sbus(sbus);

    vwr_monitor monitor("Monitor");
    monitor.clk(clk);
    monitor.rst(rst);
    monitor.wr_nrd(wr_nrd);
    monitor.idx(idx);
    monitor.d_nm(d_nm);
//    monitor.pos(pos);
//    monitor.len(len);
    monitor.lbus(lbus);
    monitor.sbus(sbus);

    sc_trace_file *tracefile;
    tracefile = sc_create_vcd_trace_file("waveforms/vwr_wave");

    sc_trace(tracefile, clk, "clk");
    sc_trace(tracefile, rst, "rst");
    sc_trace(tracefile, wr_nrd, "wr_nrd");
    sc_trace(tracefile, idx, "idx");
    sc_trace(tracefile, d_nm, "d_nm");
//    sc_trace(tracefile, pos, "pos");
//    sc_trace(tracefile, len, "len");
    sc_trace(tracefile, lbus, "lbus");
    sc_trace(tracefile, sbus, "sbus");

    sc_start(1000, SC_NS);

    sc_close_vcd_trace_file(tracefile);

    return 0;
}
