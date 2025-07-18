/*
 * Copyright EPFL 2021
 * Rafael Medina Morillas
 *
 * Description of the Program Counter Unit
 *
 */

#ifndef PC_UNIT_H_
#define PC_UNIT_H_

#include "systemc.h"

#include "cnm_base.h"

class pc_unit: public sc_module {
public:
    sc_in_clk       clk;
    sc_in<bool>     rst;
    sc_in<bool>     pc_rst;	    // Synchronous reset when end of CRF or EXIT instruction
    sc_in<bool>     count_en;   // Enables the increase of PC
#if !HW_LOOP
    sc_in<bool>     jump_en;    // Signals a jump which will translate into a decrease of PC
    sc_in<uint8_t>  jump_num;   // Amount to subtract from the PC when doing a jump
#else
    sc_in<bool>     loop_reg_en;    // Signals the update of the HW loop registers
    sc_in<uint>     loop_sta_addr;  // Address of the loop start for the HW loop
    sc_in<uint>     loop_end_addr;  // Address of the loop end for the HW loop
    sc_in<uint>     loop_num_iter;  // Number of iterations for the HW loop
#endif
    sc_out<uint>    pc_out;     // Value of the Program Counter

    sc_signal<uint> pc_nxt, pc_reg;
#if HW_LOOP
    sc_signal<uint> loop_sta_addr_reg, loop_end_addr_reg, loop_num_iter_reg;
    sc_signal<uint> loop_curr_iter_nxt, loop_curr_iter_reg;
#endif

    SC_CTOR(pc_unit) {
        SC_THREAD(clk_thread);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);

        SC_METHOD(comb_method);
        sensitive << pc_rst << count_en << pc_reg;
#if !HW_LOOP
        sensitive << jump_en << jump_num;

#else
        sensitive << loop_sta_addr_reg << loop_end_addr_reg << loop_curr_iter_reg << loop_num_iter_reg;
#endif

        pc_nxt = 0;
        pc_reg = 0;
    }

    void clk_thread();	// Performs sequential logic (and resets)
    void comb_method(); // Performs the combinational logic
};

#endif /* PC_UNIT_H_ */
