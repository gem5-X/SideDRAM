/*
 * Copyright EPFL 2021
 * Rafael Medina Morillas
 *
 * Implementation of the Program Counter Unit.
 *
 */

#include "pc_unit.h"

void pc_unit::clk_thread() {
    // Reset
    pc_reg = 0;
#if HW_LOOP
    loop_sta_addr_reg = 0;
    loop_end_addr_reg = 0;
    loop_num_iter_reg = 0;
    loop_curr_iter_reg = 0;
#endif

    wait();

    // Clocked behaviour
    while (1) {
        pc_reg = pc_nxt;
#if HW_LOOP
        loop_curr_iter_reg = loop_curr_iter_nxt;
        if (loop_reg_en->read()) {
            loop_curr_iter_reg = 0;
            loop_sta_addr_reg = loop_sta_addr;
            loop_end_addr_reg = loop_end_addr;
            loop_num_iter_reg = loop_num_iter;
        }
#endif

        wait();
    }
}

#if !HW_LOOP
void pc_unit::comb_method() {
    pc_nxt = pc_reg;

    if (pc_rst) {
        pc_nxt = 0;
    } else if (jump_en) {
        pc_nxt = pc_reg - jump_num->read();
    } else if (count_en) {
        if (pc_reg < IB_ENTRIES - 1) {
            pc_nxt = pc_reg + 1;
        } else {
            pc_nxt = 0;
        }
    }

    pc_out->write(pc_reg);
}
#else
void pc_unit::comb_method() {
    pc_nxt = pc_reg;
    loop_curr_iter_nxt = loop_curr_iter_reg;

    if (pc_rst) {
        pc_nxt = 0;
    } else if (count_en) {
        if (pc_reg < IB_ENTRIES - 1) {
            pc_nxt = pc_reg + 1;
        } else {
            pc_nxt = 0;
        }
        if (pc_reg == loop_end_addr_reg && loop_curr_iter_reg + 1 < loop_num_iter_reg) {
            pc_nxt = loop_sta_addr_reg;
            loop_curr_iter_nxt = loop_curr_iter_reg + 1;
        } else if (pc_reg == loop_end_addr_reg && loop_curr_iter_reg + 1 == loop_num_iter_reg) {
            loop_curr_iter_nxt = 0;
        }
    }
    pc_out->write(pc_reg);
}
#endif
