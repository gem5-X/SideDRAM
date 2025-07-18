/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Implementation of the sequencer that generates the microcode indices to perform multiplication
 * through shifts and additions with a CSD multiplier.
 *
 */

#include "mult_sequencer.h"

ostream& operator<< (ostream& os, const MS_FSM& rhs) {
    os << uint(rhs);
    return os;
}

void sc_trace (sc_trace_file*& tf, const MS_FSM& store, std::string nm) {
    sc_trace(tf, uint(store), nm);
}

sc_bv<SA_MAX_SHIFT*2> fill_csd_window (uint idx, sc_bv<CSD_64B*64+2*SA_MAX_SHIFT> csd) {
    sc_bv<SA_MAX_SHIFT*2> window = 0;
    switch (idx) {
        case 0:     window = csd.range(0+SA_MAX_SHIFT*2-1, 0);      break;
        case 2:     window = csd.range(2+SA_MAX_SHIFT*2-1, 2);      break;
        case 4:     window = csd.range(4+SA_MAX_SHIFT*2-1, 4);      break;
        case 6:     window = csd.range(6+SA_MAX_SHIFT*2-1, 6);      break;
        case 8:     window = csd.range(8+SA_MAX_SHIFT*2-1, 8);      break;
        case 10:    window = csd.range(10+SA_MAX_SHIFT*2-1, 10);    break;
        case 12:    window = csd.range(12+SA_MAX_SHIFT*2-1, 12);    break;
        case 14:    window = csd.range(14+SA_MAX_SHIFT*2-1, 14);    break;
        case 16:    window = csd.range(16+SA_MAX_SHIFT*2-1, 16);    break;
        case 18:    window = csd.range(18+SA_MAX_SHIFT*2-1, 18);    break;
        case 20:    window = csd.range(20+SA_MAX_SHIFT*2-1, 20);    break;
        case 22:    window = csd.range(22+SA_MAX_SHIFT*2-1, 22);    break;
        case 24:    window = csd.range(24+SA_MAX_SHIFT*2-1, 24);    break;
        case 26:    window = csd.range(26+SA_MAX_SHIFT*2-1, 26);    break;
        case 28:    window = csd.range(28+SA_MAX_SHIFT*2-1, 28);    break;
        case 30:    window = csd.range(30+SA_MAX_SHIFT*2-1, 30);    break;
        case 32:    window = csd.range(32+SA_MAX_SHIFT*2-1, 32);    break;
        case 34:    window = csd.range(34+SA_MAX_SHIFT*2-1, 34);    break;
        case 36:    window = csd.range(36+SA_MAX_SHIFT*2-1, 36);    break;
        case 38:    window = csd.range(38+SA_MAX_SHIFT*2-1, 38);    break;
        case 40:    window = csd.range(40+SA_MAX_SHIFT*2-1, 40);    break;
        case 42:    window = csd.range(42+SA_MAX_SHIFT*2-1, 42);    break;
        case 44:    window = csd.range(44+SA_MAX_SHIFT*2-1, 44);    break;
        case 46:    window = csd.range(46+SA_MAX_SHIFT*2-1, 46);    break;
        case 48:    window = csd.range(48+SA_MAX_SHIFT*2-1, 48);    break;
        default:    window = csd.range(0+SA_MAX_SHIFT*2-1, 0);      break;
    }
    return window;
}

void mult_sequencer::clk_thread() {
    // Reset
    state_reg = MS_FSM::IDLE;
    idx_reg = 0;
    size_reg = SWSIZE::INV;
    src_reg = OPC_STORAGE::SA;
    src_n_reg = 0;
    dst_reg = OPC_STORAGE::R3;
    dst_n_reg = 0;
    csd_reg = 0;
    len_reg = CSD_BITS / 2;

    wait();

    // Clocked behaviour
    while (1) {
        state_reg = state_nxt;
        idx_reg = (idx_rst ? 0 : idx_nxt);
        size_reg = size_nxt;
        src_reg = src_nxt;
        src_n_reg = src_n_nxt;
        dst_reg = dst_nxt;
        dst_n_reg = dst_n_nxt;
        csd_reg = csd_nxt;
        len_reg = len_nxt;

        wait();
    }
}

void mult_sequencer::fsm_method() {
    // Default signals for registers
    state_nxt = state_reg;
    idx_rst = false;

    MS_FSM state_aux = state_reg;
    state_out = (uint) state_aux;

    switch (state_reg) {
        case MS_FSM::IDLE:
            // If enable, start new multiplication (not a continuation of the current one)
            if (enable->read()) {
                if (first_found)    state_nxt = MS_FSM::GEN_SA;         // Found non-zero bit in first window
                else                state_nxt = MS_FSM::FIND_NONZERO;   // Not found non-zero bit in first window
            }
        break;
        case MS_FSM::FIND_NONZERO:  // Searching for first non-zero bit in CSD multiplier
            if (first_found) {  // First non-zero found
                if (idx_nxt < 2*len_reg) {
                    state_nxt = MS_FSM::GEN_SA; // Continue multiplication with shift and add generation
                } else {    // First non-zero bit is MSB
                    if (dst_reg == OPC_STORAGE::R3) {
                        state_nxt = MS_FSM::IDLE;       // Result already in R3
                        idx_rst = true;
                    }
                    else {
                        state_nxt = MS_FSM::WRITE_NOR3; // Result to be written to R1/R2
                    }
                }
            } else if (idx_nxt >= 2*len_reg) {
                state_nxt = MS_FSM::ZERO_MULT;  // All bits in multiplier are 0
            }
        break;
        case MS_FSM::ZERO_MULT:     // All bits are zero in CSD multiplier
            if (dst_reg == OPC_STORAGE::R3) {
                state_nxt = MS_FSM::IDLE;       // Result already in R3
                idx_rst = true;
            }
            else {
                state_nxt = MS_FSM::WRITE_NOR3; // Result to be written to R1/R2
            }
        break;
        case MS_FSM::GEN_SA:        // Generating sequence of shifts and add after first non-zero bit
            if (idx_nxt >= 2*len_reg) {
                if (dst_reg == OPC_STORAGE::R3) {
                    state_nxt = MS_FSM::IDLE;       // Result already in R3
                    idx_rst = true;
                }
                else {
                    state_nxt = MS_FSM::WRITE_NOR3; // Result to be written to R1/R2
                }
            }
        break;
        case MS_FSM::WRITE_NOR3:    // Write multiplication result to R1 and/or R2
            state_nxt = MS_FSM::IDLE;
            idx_rst = true;
        break;
        default:
            state_nxt = MS_FSM::IDLE;
            idx_rst = true;
        break;
    }
}

void mult_sequencer::comb_method() {
    uint i;
    sc_bv<64> parse_aux;
    sc_bv<CSD_64B*64+2*SA_MAX_SHIFT> csd_aux;
    sc_bv<SA_MAX_SHIFT*2> window;

    // Default signals for registers
    idx_nxt = idx_reg;
    size_nxt = size_reg;
    src_nxt = src_reg;
    src_n_nxt = src_n_reg;
    dst_nxt = dst_reg;
    dst_n_nxt = dst_n_reg;
    csd_nxt = csd_reg;
    len_nxt = len_reg;

    // Default signal
    first_found = false;

    // Default ports
    mov_valid->write(false);
    mov_src->write(OPC_STORAGE::SA);
    mov_src_n->write(0);
    sa_valid->write(false);
#if (INSTR_FORMAT == BASE_FORMAT)
    sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::NOP)]));
    sa_size->write(size_reg);
    sa_shift->write(0);
#elif (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)
    sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::NOP)][0]));
    sa_size->write(size_reg);
#endif
    sa_src0->write(OPC_STORAGE::R3);
    sa_dst->write(OPC_STORAGE::R3);
    mov_dst_n->write(0);

#if STATIC_CSD_LEN
    if (csd_len_en->read()) {
        len_nxt = csd_len->read();
    }
#endif

    switch (state_reg) {
        case MS_FSM::IDLE:
            if (enable->read()) {
                // If starting new multiplication, register CSD input and DST
                size_nxt = sw_size->read();
                src_nxt = src->read();
                src_n_nxt = src_n->read();
                dst_nxt = dst->read();
                dst_n_nxt = dst_n->read();
                csd_aux = 0;
                for (uint i = 0; i < CSD_64B; i++) {
                    parse_aux = csd_in[i]->read();
                    csd_aux.range((i+1)*64-1, i*64) = parse_aux;
                }
                csd_nxt = csd_aux;
#if !STATIC_CSD_LEN
                len_nxt = csd_len->read();
#endif

                // If starting new multiplication, observe the first bits directly from csd_in
                window = fill_csd_window(0, csd_aux);

                // Update index for next window in case non-zero bits are not found
                idx_nxt = idx_reg + SA_MAX_SHIFT*2;

                // Find first non-zero bit
                for (i = 0; i < SA_MAX_SHIFT*2; i += 2) {
                    if (window.range(i+1,i) != CSD_ZERO) {  // Non zero bit
                        mov_valid->write(true);
                        mov_src->write(src->read());
                        mov_src_n->write(src_n->read());
                        if (window.range(i+1,i) == CSD_POSONE) {    // +1
#if (INSTR_FORMAT == BASE_FORMAT)
                            sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::NOP)]));
                            sa_shift->write(0);
#elif (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)
                            sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::NOP)][0]));
#endif
                        } else {                                    // -1
#if (INSTR_FORMAT == BASE_FORMAT)
                            sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::INV)]));
                            sa_shift->write(0);
#elif (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)
                            sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::INV)][0]));
#endif
                        }
                        sa_valid->write(true);
                        sa_size->write(sw_size->read());
                        sa_src0->write(OPC_STORAGE::VWR);
                        sa_dst->write(OPC_STORAGE::R3);
                        first_found = true;
                        idx_nxt = idx_reg + i + 2;  // Update index for next window
                        break;
                    }
                }
            }
        break;
        case MS_FSM::FIND_NONZERO:  // Searching for first non-zero bit in CSD multiplier
            // Observe next bits
            csd_aux = csd_reg;
            window = fill_csd_window(idx_reg, csd_aux);

            // Update index for next window in case non-zero bits are not found
            idx_nxt = idx_reg + SA_MAX_SHIFT*2;

            // Find first non-zero bit
            for (i = 0; i < SA_MAX_SHIFT*2; i += 2) {
                if (window.range(i+1,i) != CSD_ZERO) {  // Non zero bit
                    mov_valid->write(true);
                    mov_src->write(src_reg);
                    mov_src_n->write(src_n_reg);
                    if (window.range(i+1,i) == CSD_POSONE) {    // +1
#if (INSTR_FORMAT == BASE_FORMAT)
                            sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::NOP)]));
                            sa_shift->write(0);
#elif (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)
                            sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::NOP)][0]));
#endif
                    } else {                                    // -1
#if (INSTR_FORMAT == BASE_FORMAT)
                            sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::INV)]));
                            sa_shift->write(0);
#elif (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)
                            sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::INV)][0]));
#endif
                    }
                    sa_valid->write(true);
                    sa_src0->write(OPC_STORAGE::VWR);
                    sa_dst->write(OPC_STORAGE::R3);
                    first_found = true;
                    idx_nxt = idx_reg + i + 2;  // Update index for next window
                    break;
                }
            }
        break;
        case MS_FSM::ZERO_MULT:     // All bits are zero in CSD multiplier
            sa_valid->write(true);
#if (INSTR_FORMAT == BASE_FORMAT)
            sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::NOP)]));
            sa_shift->write(0);
#elif (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)
            sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::NOP)][0]));
#endif
            sa_src0->write(OPC_STORAGE::ZERO);
            sa_dst->write(OPC_STORAGE::R3);
        break;
        case MS_FSM::GEN_SA:        // Generating sequence of shifts and add after first non-zero bit
            // Observe next bits
            csd_aux = csd_reg;
            window = fill_csd_window(idx_reg, csd_aux);

            // Update index for next window in case non-zero bits are not found
            idx_nxt = idx_reg + SA_MAX_SHIFT*2;

            sa_valid->write(true);
            // Default max shift in case non-zero bit are not found
#if (INSTR_FORMAT == BASE_FORMAT)
            sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::NOP)]));
            sa_shift->write(((len_reg-idx_reg/2) < 3) ? (len_reg-idx_reg/2) : 3);
#elif (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)
            sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::NOP)][((len_reg-idx_reg/2) < 3) ? (len_reg-idx_reg/2) : 3]));
#endif
            sa_src0->write(OPC_STORAGE::R3);
            sa_dst->write(OPC_STORAGE::R3);

            // Find next non-zero bit
            for (i = 0; i < SA_MAX_SHIFT*2; i += 2) {
                if (window.range(i+1,i) != CSD_ZERO) {  // Non zero bit
                    if (window.range(i+1,i) == CSD_POSONE) {    // +1
#if (INSTR_FORMAT == BASE_FORMAT)
                        sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::ADD)]));
                        sa_shift->write(i/2 + 1);
#elif (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)
                        sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::ADD)][i/2 + 1]));
#endif
                    } else {                                    // -1
#if (INSTR_FORMAT == BASE_FORMAT)
                        sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::SUB)]));
                        sa_shift->write(i/2 + 1);
#elif (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)
                        sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::SUB)][i/2 + 1]));
#endif
                    }
                    idx_nxt = idx_reg + i + 2;  // Update index for next window
                    break;
                }
            }
        break;
        case MS_FSM::WRITE_NOR3:    // Write multiplication result to R1 and/or R2
            sa_valid->write(true);
#if (INSTR_FORMAT == BASE_FORMAT)
            sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::NOP)]));
            sa_shift->write(0);
#elif (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)
            sa_index->write(uint(mult_seq_microinstr[uint(ADDOP::NOP)][0]));
#endif
            sa_src0->write(OPC_STORAGE::R3);
            if (is_vwr(dst_reg)) {
                mov_valid->write(true);
                mov_src->write(OPC_STORAGE::SA);
            }
            sa_dst->write(dst_reg);
            mov_dst_n->write(dst_n_reg);
        break;
        default:
        break;
    }
}
