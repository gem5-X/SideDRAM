#include "softs_driver.h"
#include "softs_monitor.h"
#include "../shift_and_add.h"
#include "../pack_and_mask.h"
#include <string>

int sc_main(int argc, char *argv[]) {
    sc_clock                clk("clk", 10, SC_NS);
    sc_signal<bool>         rst;
    // Shift & Add
    sc_signal<bool>         SA_en;
    sc_signal<uint>         SA_shift;
    sc_signal<uint>         SA_size;
    sc_signal<bool>         SA_s_na;
    sc_signal<uint64_t>     SA_op1[WORD_64B];
    sc_signal<uint64_t>     SA_op2[WORD_64B];
    sc_signal<uint64_t>     SA_out[WORD_64B];
    // Pack & Mask
    sc_signal<bool>         PM_en;
    sc_signal<uint64_t>     PM_w1[WORD_64B];
    sc_signal<uint64_t>     PM_w2[WORD_64B];
    sc_signal<uint>         PM_in_size;
    sc_signal<uint>         PM_out_size;
    sc_signal<uint>         PM_in_start;
    sc_signal<uint64_t>     PM_mask_in[MASK_64B];
    sc_signal<uint8_t>      PM_op_sel;
    sc_signal<uint>         PM_shift;
    sc_signal<uint64_t>     PM_out[WORD_64B];

    shift_and_add saut("Shift&AddUnderTest");
    saut.clk(clk);
    saut.rst(rst);
    saut.compute_en(SA_en);
    saut.shift(SA_shift);
    saut.size(SA_size);
    saut.s_na(SA_s_na);
    for (uint i = 0; i < WORD_64B; i++) {
        saut.op1[i](SA_op1[i]);
        saut.op2[i](SA_op2[i]);
        saut.output[i](SA_out[i]);
    }

    pack_and_mask pmut("Pack&MaskUnderTest");
    pmut.clk(clk);
    pmut.rst(rst);
    pmut.compute_en(PM_en);
    for (uint i = 0; i < WORD_64B; i++) {
        pmut.w1[i](PM_w1[i]);
        pmut.w2[i](PM_w2[i]);
    }
    pmut.in_size(PM_in_size);
    pmut.out_size(PM_out_size);
    pmut.in_start(PM_in_start);
    for (uint i = 0; i < MASK_64B; i++) {
        pmut.mask_in[i](PM_mask_in[i]);
    }
    pmut.op_sel(PM_op_sel);
    pmut.shift(PM_shift);
    for (uint i = 0; i < WORD_64B; i++) {
        pmut.output[i](PM_out[i]);
    }

    softs_driver driver("Driver");
    driver.rst(rst);
    driver.SA_en(SA_en);
    driver.SA_shift(SA_shift);
    driver.SA_size(SA_size);
    driver.SA_s_na(SA_s_na);
    for (uint i = 0; i < WORD_64B; i++) {
        driver.SA_op1[i](SA_op1[i]);
        driver.SA_op2[i](SA_op2[i]);
    }
    driver.PM_en(PM_en);
    for (uint i = 0; i < WORD_64B; i++) {
        driver.PM_w1[i](PM_w1[i]);
        driver.PM_w2[i](PM_w2[i]);
    }
    driver.PM_in_size(PM_in_size);
    driver.PM_out_size(PM_out_size);
    driver.PM_in_start(PM_in_start);
    for (uint i = 0; i < MASK_64B; i++) {
        driver.PM_mask_in[i](PM_mask_in[i]);
    }
    driver.PM_op_sel(PM_op_sel);
    driver.PM_shift(PM_shift);

    softs_monitor monitor("Monitor");
    monitor.clk(clk);
    monitor.rst(rst);
    monitor.SA_en(SA_en);
    monitor.SA_shift(SA_shift);
    monitor.SA_size(SA_size);
    monitor.SA_s_na(SA_s_na);
    for (uint i = 0; i < WORD_64B; i++) {
        monitor.SA_op1[i](SA_op1[i]);
        monitor.SA_op2[i](SA_op2[i]);
        monitor.SA_out[i](SA_out[i]);
    }
    monitor.PM_en(PM_en);
    for (uint i = 0; i < WORD_64B; i++) {
        monitor.PM_w1[i](PM_w1[i]);
        monitor.PM_w2[i](PM_w2[i]);
    }
    monitor.PM_in_size(PM_in_size);
    monitor.PM_out_size(PM_out_size);
    monitor.PM_in_start(PM_in_start);
    for (uint i = 0; i < MASK_64B; i++) {
        monitor.PM_mask_in[i](PM_mask_in[i]);
    }
    monitor.PM_op_sel(PM_op_sel);
    monitor.PM_shift(PM_shift);
    for (uint i = 0; i < WORD_64B; i++) {
        monitor.PM_out[i](PM_out[i]);
    }

    sc_trace_file *tracefile;
    tracefile = sc_create_vcd_trace_file("waveforms/softs_wave");

    sc_trace(tracefile, clk, "clk");
    sc_trace(tracefile, rst, "rst");
    sc_trace(tracefile, SA_en, "SA_en");
    sc_trace(tracefile, SA_shift, "SA_shift");
    sc_trace(tracefile, SA_size, "SA_size");
    sc_trace(tracefile, SA_s_na, "SA_s_na");
    for (uint i = 0; i < WORD_64B; i++) {
        sc_trace(tracefile, SA_op1[i], "SA_op1_" + std::to_string(i));
        sc_trace(tracefile, SA_op2[i], "SA_op2_" + std::to_string(i));
        sc_trace(tracefile, SA_out[i], "SA_out_" + std::to_string(i));
    }
    sc_trace(tracefile, PM_en, "PM_en");
    for (uint i = 0; i < WORD_64B; i++) {
        sc_trace(tracefile, PM_w1[i], "PM_w1_" + std::to_string(i));
        sc_trace(tracefile, PM_w2[i], "PM_w2_" + std::to_string(i));
    }
    sc_trace(tracefile, PM_in_size, "PM_in_size");
    sc_trace(tracefile, PM_out_size, "PM_out_size");
    sc_trace(tracefile, PM_in_start, "PM_in_start");
    for (uint i = 0; i < MASK_64B; i++) {
        sc_trace(tracefile, PM_mask_in[i], "PM_mask_in_" + std::to_string(i));
    }
    sc_trace(tracefile, PM_op_sel, "PM_op_sel");
    sc_trace(tracefile, PM_shift, "PM_shift");
    for (uint i = 0; i < WORD_64B; i++) {
        sc_trace(tracefile, PM_out[i], "PM_out_" + std::to_string(i));
    }

    sc_start(2200, SC_NS);

    sc_close_vcd_trace_file(tracefile);

    return 0;
}
