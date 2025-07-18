/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Description of the sequencer that generates the microcode indices to perform multiplication
 * through shifts and additions with a CSD multiplier.
 *
 */

#ifndef SRC_MULT_SEQUENCER_H_
#define SRC_MULT_SEQUENCER_H_

#include "systemc.h"

#include "cnm_base.h"

enum class MS_FSM : uint {IDLE, FIND_NONZERO, ZERO_MULT, GEN_SA, WRITE_NOR3};

ostream& operator<< (ostream& os, const MS_FSM& rhs);

void sc_trace (sc_trace_file*& tf, const MS_FSM& store, std::string nm);

sc_bv<SA_MAX_SHIFT*2> fill_csd_window (uint idx, sc_bv<CSD_64B*64+2*SA_MAX_SHIFT> csd);

class mult_sequencer: public sc_module {
public:
    sc_in_clk           clk;
    sc_in<bool>         rst;
    sc_in<bool>         enable;             // Signals if macroinstruction buffer is pointing here
    sc_in<SWSIZE>       sw_size;            // Subword size specified by macroinstruction
    sc_in<OPC_STORAGE>  src;                // SRC VWR specified by macroinstruction
    sc_in<uint>         src_n;              // SRC index specified by macroinstruction
    sc_in<OPC_STORAGE>  dst;                // DST specified by macroinstruction
    sc_in<uint>         dst_n;              // DST index specified by macroinstruction
    sc_in<uint64_t>     csd_in[CSD_64B];    // CSD multiplier input
    sc_in<uint8_t>      csd_len;            // Length of CSD multiplier input
#if STATIC_CSD_LEN
    sc_in<bool>         csd_len_en;         // Signals the update of the CSD multiplicand length
#endif
    sc_out<bool>        mov_valid;          // Signals if MOV commands are valid
    sc_out<OPC_STORAGE> mov_src;            // Sets SRC for the current microinstruction
    sc_out<uint>        mov_src_n;          // Sets SRC_N for the current microinstruction
    sc_out<bool>        sa_valid;           // Signals if S&A index is valid
    sc_out<uint>        sa_index;           // Index for the S&A DLB microcode
    sc_out<SWSIZE>      sa_size;            // Size for the S&A microinstruction
#if INSTR_FORMAT == BASE_FORMAT
    sc_out<uint>        sa_shift;           // Sets shift for the current microinstruction
#endif
    sc_out<OPC_STORAGE> sa_src0;            // Sets SRC0 for the current microinstruction
    sc_out<OPC_STORAGE> sa_dst;             // Sets DST for the current microinstruction
    sc_out<uint>        mov_dst_n;          // Sets DST_N for the current microinstruction

    // Internal signals
    sc_signal<bool>                                 idx_rst;                // Reset index if going back to IDLE state
    sc_signal<bool>                                 first_found;            // Signals finding of first non-zero bit

    sc_signal<MS_FSM>                               state_nxt, state_reg;   // FSM state
    sc_signal<uint>                                 idx_nxt, idx_reg;       // Next lower index of the window
    sc_signal<SWSIZE>                               size_nxt, size_reg;     // Subword size passed by macroinstruction
    sc_signal<OPC_STORAGE>                          src_nxt, src_reg;       // SRC passed by macroinstruction
    sc_signal<uint>                                 src_n_nxt, src_n_reg;   // SRC index passed by macroinstruction
    sc_signal<OPC_STORAGE>                          dst_nxt, dst_reg;       // DST passed by macroinstruction
    sc_signal<uint>                                 dst_n_nxt, dst_n_reg;   // DST index passed by macroinstruction
    sc_signal<sc_bv<CSD_64B*64+2*SA_MAX_SHIFT> >    csd_nxt, csd_reg;       // CSD multiplicand
    sc_signal<uint8_t>                              len_nxt, len_reg;       // CSD multiplicand length
    sc_signal<uint>                                 state_out;

    SC_HAS_PROCESS(mult_sequencer);
    mult_sequencer(sc_module_name name) : sc_module(name) {

        SC_THREAD(clk_thread);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);

        SC_METHOD(fsm_method);
        sensitive << enable;
        sensitive << state_reg << first_found << idx_nxt << len_reg << dst_reg << dst_n_reg;

        SC_METHOD(comb_method);
        sensitive << enable << sw_size << dst << dst_n << src << src_n;
        for (uint i = 0; i < CSD_64B; i++)
            sensitive << csd_in[i];
        sensitive << state_reg << csd_reg << csd_len << idx_reg << size_reg << len_reg;
        sensitive << src_reg << src_n_reg << dst_reg << dst_n_reg;
#if STATIC_CSD_LEN
        sensitive << csd_len_en;
#endif
    }

    void clk_thread();  // Manages registers
    void fsm_method();  // FSM governing the behavior of the multiplication sequencer
    void comb_method(); // Logic to generate the microinstruction sequence for multiplication
};

#endif /* SRC_MULT_SEQUENCER_H_ */
