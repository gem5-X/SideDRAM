/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the Interface Unit for SoftSIMD.
 *
 */

#ifndef INTERFACE_UNIT_SOFTSIMD_H_
#define INTERFACE_UNIT_SOFTSIMD_H_

#include "cnm_base.h"

class interface_unit: public sc_module {
public:
    sc_in_clk   clk;
    sc_in<bool> rst;

    // Interface input
    sc_in<bool>                 RD;         // DRAM read command
    sc_in<bool>                 WR;         // DRAM write command
    sc_in<bool>                 ACT;        // DRAM activate command
//    sc_in<bool>                       RSTB;       //
    sc_in<bool>                 AB_mode;    // Signals if the All-Banks mode is enabled
    sc_in<bool>                 pim_mode;   // Signals if the PIM mode is enabled
    sc_in<sc_uint<BANK_BITS> >  bank_addr;  // Address of the bank
    sc_in<sc_uint<ROW_BITS> >   row_addr;   // Address of the bank row
    sc_in<sc_uint<COL_BITS> >   col_addr;   // Address of the bank column
    sc_in<uint64_t>             DQ;         // Data input from DRAM controller (output makes no sense here)

    // Interface output
    sc_out<bool>                rf_access;  // Enables access to the RFs
//    sc_out<bool>              rf_wr_nrd;  // Signals if reading (low) or writing (high)
    sc_out<bool>                decode_en;  // Enables decoding of the next instruction
    sc_out<uint64_t>            data_out;   // Data to the registers
#if HW_LOOP
    sc_out<bool>                loop_reg_en;    // Signals the update of the HW loop registers
    sc_out<uint>                loop_sta_addr;  // Address of the loop start for the HW loop
    sc_out<uint>                loop_end_addr;  // Address of the loop end for the HW loop
    sc_out<uint>                loop_num_iter;  // Number of iterations for the HW loop
#endif
#if STATIC_CSD_LEN
    sc_out<bool>                csd_len_en;     // Signals the update of the CSD multiplicand length
    sc_out<uint8_t>             csd_len;        // New length for the CSD multiplicand
#endif

    // Internal signals and variables
    SC_HAS_PROCESS(interface_unit);
    interface_unit(sc_module_name name) : sc_module(name) {
        SC_METHOD(comb_method);
        sensitive << RD << WR << ACT << AB_mode << pim_mode;
        sensitive << bank_addr << row_addr << col_addr << DQ;
    }

    void comb_method(); // Performs the combinational logic
};

#endif /* INTERFACE_UNIT_SOFTSIMD_H_ */
