#include "../cnm_base.h"

#if MIXED_SIM

#include "cp_driver.h"

#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <deque>
#include <assert.h>

using namespace std;

void cp_driver::driver_thread() {
    
    std::mt19937_64 gen(0); // Standard mersenne_twister_engine seeded
    int i, j, curCycle;
    uint itt_idx, imm, shift_sa, rf_n, src0_n, pack_sta, shift_pm, dst_n, dst_n_sa;
    OPC_STORAGE dst, src, src0;
    SWSIZE sw_len = SWSIZE::INV;
    SWREPACK repack = SWREPACK::INV;
    sc_uint<ROW_BITS> row_to_write(0);
    sc_uint<COL_BITS> col_to_write(0);
    sc_uint<RF_SEL_BITS> rf_sel(0);
    sc_uint<RF_ADDR_BITS> rf_addr(0);
    sc_uint<64> dq_data(0);

    // Warm-up random number generator
    for (int i = 0; i < 1000; i++) {
        gen();
    }

    // Initial reset
    curCycle = 0;
    rst->write(SC_LOGIC_0);
    RD->write(SC_LOGIC_0);
    WR->write(SC_LOGIC_0);
    ACT->write(SC_LOGIC_0);
    AB_mode->write(SC_LOGIC_0);
    pim_mode->write(SC_LOGIC_0);
    bank_addr->write(0);
    row_addr->write(0);
    col_addr->write(0);
    DQ->write(0);

    wait(CLK_PERIOD / 2, RESOLUTION);
    rst->write(SC_LOGIC_1);
    wait(0, RESOLUTION);
    wait(CLK_PERIOD / 2 +  CLK_PERIOD / 10, RESOLUTION);
    curCycle++;

    // All banks mode and PIM mode enabled
    AB_mode->write(SC_LOGIC_1);
    pim_mode->write(SC_LOGIC_1);

#if (TESTED_INSTRS == TEST_IDLE)
    // RD, WR always off, DRAM addresses and DQ changing
    while (curCycle < TESTED_CYCLES) {
        // Change DRAM addresses and DQ with a 1/10 probability
        if (gen() % 10 == 0) {
            bank_addr->write(gen() % (1 << BANK_BITS));
            row_addr->write(gen() % (1 << ROW_BITS));
            col_addr->write(gen() % (1 << COL_BITS));
            DQ->write(gen());
        }
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
#elif (TESTED_INSTRS == TEST_WRF_CSDLEN_LOOP)
    // WR on, DRAM addresses and DQ changing to write CSD_LEN or LOOP
    while (curCycle < TESTED_CYCLES) {
        WR->write(SC_LOGIC_1);
        // 1/10 is a write to LOOP
        if (gen() % 10 == 0) {
            rf_sel = uint(RF_SEL::LOOP_REG);
            rf_addr = 0;
            write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
            dq_data = loop_to_dq(gen() % IB_ENTRIES, gen() % IB_ENTRIES, gen() % (1 << 16));
        // 9/10 is a write to CSD_LEN
        } else {
            rf_sel = uint(RF_SEL::CSD_LEN);
            rf_addr = 0;
            write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
            dq_data = gen() % (CSD_BITS/2); // CSD_LEN
        }
        bank_addr->write(gen() % (1 << BANK_BITS));
        row_addr->write(row_to_write);
        col_addr->write(col_to_write);
        DQ->write(dq_data);
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
#elif (TESTED_INSTRS == TEST_WRF_IB)
    // WR on, DRAM addresses and DQ changing to write IB
    while (curCycle < TESTED_CYCLES) {
        WR->write(SC_LOGIC_1);
        rf_sel = uint(RF_SEL::IB);
        rf_addr = (i++) % IB_ENTRIES;
        write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
        itt_idx = gen() % (uint(MACRO_IDX::MAX));
        imm = gen() % (1 << MACRO_IMM_BITS);
        dst = OPC_STORAGE(gen() % (1 << MACRO_DST_BITS));
        src = OPC_STORAGE(gen() % (1 << MACRO_SRC_BITS));
        sw_len = random_sw_size[gen() % POSSIBLE_SW_SIZES];
        shift_sa = gen() % (1 << MACRO_SHIFTSA_BITS);
        rf_n = gen() % (1 << MACRO_RF_N_BITS);
        src0_n = gen() % (1 << MACRO_SRC0_N_BITS);
        src0 = OPC_STORAGE(gen() % (1 << MACRO_SRC_BITS));
        repack = random_repack[gen() % POSSIBLE_REPACKS];
        pack_sta = gen() % (1 << MACRO_PACK_STA_BITS);
        shift_pm = 0;
        dst_n = gen() % (1 << MACRO_DST_N_BITS);
        dst_n_sa = gen() % (1 << MACRO_DST_N_SA_BITS);
        dq_data = build_macroinstr(itt_idx, imm, dst, src, sw_len, shift_sa, rf_n,
                             src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
        bank_addr->write(gen() % (1 << BANK_BITS));
        row_addr->write(row_to_write);
        col_addr->write(col_to_write);
        DQ->write(dq_data);
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
#elif (TESTED_INSTRS == TEST_WRF_CSD)
    // WR on, DRAM addresses and DQ changing to write CSD
    while (curCycle < TESTED_CYCLES) {
        WR->write(SC_LOGIC_1);
        rf_sel = uint(RF_SEL::CSDRF);
        rf_addr = (i++) % CSD_ENTRIES;
        write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
        dq_data = generate_random_csd(TESTED_CSD_LEN, gen);
        bank_addr->write(gen() % (1 << BANK_BITS));
        row_addr->write(row_to_write);
        col_addr->write(col_to_write);
        DQ->write(dq_data);
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
#elif (TESTED_INSTRS == TEST_NOP)
    // Write NOPs 1 cycle
    for (i = 0; i < TEST_LOOP_LEN; i++) {
        WR->write(SC_LOGIC_1);
        rf_sel = uint(RF_SEL::IB);
        rf_addr = i;
        write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
        itt_idx = uint(MACRO_IDX::NOP);
        imm = 1;
        dq_data = build_macroinstr(itt_idx, imm, dst, src, sw_len, shift_sa, rf_n,
                             src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
        bank_addr->write(gen() % (1 << BANK_BITS));
        row_addr->write(row_to_write);
        col_addr->write(col_to_write);
        DQ->write(dq_data);
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
    // Write a loop of NOPs
    WR->write(SC_LOGIC_1);
    rf_sel = uint(RF_SEL::LOOP_REG);
    rf_addr = 0;
    write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
    dq_data = loop_to_dq(0, TEST_LOOP_LEN-1, 0xFFFF);
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(row_to_write);
    col_addr->write(col_to_write);
    DQ->write(dq_data);
    wait(CLK_PERIOD, RESOLUTION);
    curCycle++;
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(gen() % (1 << (ROW_BITS-1)));
    col_addr->write(gen() % (1 << COL_BITS));
    DQ->write(gen());
    // RD on, DRAM addresses and DQ changing with 1/10 probability
    while (curCycle < TESTED_CYCLES) {
        WR->write(SC_LOGIC_0);
        RD->write(SC_LOGIC_1);
        // Change DRAM addresses and DQ with a 1/10 probability
        if (gen() % 10 == 0) {
            bank_addr->write(gen() % (1 << BANK_BITS));
            row_addr->write(gen() % (1 << (ROW_BITS-1)));
            col_addr->write(gen() % (1 << COL_BITS));
            DQ->write(gen());
        }
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
#elif (TESTED_INSTRS == TEST_RLB)
    // Write RLBs to VWR 0 and 1
    for (i = 0; i < TEST_LOOP_LEN; i++) {
        WR->write(SC_LOGIC_1);
        rf_sel = uint(RF_SEL::IB);
        rf_addr = i;
        write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
        itt_idx = uint(MACRO_IDX::RLB);
        imm = 0;    // Mask is ALL_WORDS
        src = OPC_STORAGE::DRAM;
        dst = (i%2) ? OPC_STORAGE::VWR1 : OPC_STORAGE::VWR0;
        dst_n = gen() % (1 << MACRO_DST_N_BITS);
        dq_data = build_macroinstr(itt_idx, imm, dst, src, sw_len, shift_sa, rf_n,
                             src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
        bank_addr->write(gen() % (1 << BANK_BITS));
        row_addr->write(row_to_write);
        col_addr->write(col_to_write);
        DQ->write(dq_data);
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
    // Write a loop of RLBs
    WR->write(SC_LOGIC_1);
    rf_sel = uint(RF_SEL::LOOP_REG);
    rf_addr = 0;
    write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
    dq_data = loop_to_dq(0, TEST_LOOP_LEN-1, 0xFFFF);
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(row_to_write);
    col_addr->write(col_to_write);
    DQ->write(dq_data);
    wait(CLK_PERIOD, RESOLUTION);
    curCycle++;
    // RD on and off, DRAM addresses changing every two cycles
    while (curCycle < TESTED_CYCLES) {
        WR->write(SC_LOGIC_0);
        RD->write(SC_LOGIC_1);
        bank_addr->write(gen() % (1 << BANK_BITS));
        row_addr->write(gen() % (1 << (ROW_BITS-1)));
        col_addr->write(gen() % (1 << COL_BITS));
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
        RD->write(SC_LOGIC_0);
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
#elif (TESTED_INSTRS == TEST_WLB)
    // Write WLBs from VWR 0 and 1
    for (i = 0; i < TEST_LOOP_LEN; i++) {
        WR->write(SC_LOGIC_1);
        rf_sel = uint(RF_SEL::IB);
        rf_addr = i;
        write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
        itt_idx = uint(MACRO_IDX::WLB);
        imm = 0;    // Mask is ALL_WORDS
        src = (i%2) ? OPC_STORAGE::VWR1 : OPC_STORAGE::VWR0;
        src0_n = gen() % (1 << MACRO_DST_N_BITS);
        dst = OPC_STORAGE::DRAM;
        dq_data = build_macroinstr(itt_idx, imm, dst, src, sw_len, shift_sa, rf_n,
                             src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
        bank_addr->write(gen() % (1 << BANK_BITS));
        row_addr->write(row_to_write);
        col_addr->write(col_to_write);
        DQ->write(dq_data);
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
    // Write a loop of RLBs
    WR->write(SC_LOGIC_1);
    rf_sel = uint(RF_SEL::LOOP_REG);
    rf_addr = 0;
    write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
    dq_data = loop_to_dq(0, TEST_LOOP_LEN-1, 0xFFFF);
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(row_to_write);
    col_addr->write(col_to_write);
    DQ->write(dq_data);
    wait(CLK_PERIOD, RESOLUTION);
    curCycle++;
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(gen() % (1 << (ROW_BITS-1)));
    col_addr->write(gen() % (1 << COL_BITS));
    DQ->write(gen());
    // RD on, DRAM addresses changing
    while (curCycle < TESTED_CYCLES) {
        WR->write(SC_LOGIC_1);
        bank_addr->write(gen() % (1 << BANK_BITS));
        row_addr->write(gen() % (1 << (ROW_BITS-1)));
        col_addr->write(gen() % (1 << COL_BITS));
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
#elif (TESTED_INSTRS == TEST_VMV)
    // Write VMVs from VWR 0 and 1
    for (i = 0; i < TEST_LOOP_LEN; i++) {
        WR->write(SC_LOGIC_1);
        rf_sel = uint(RF_SEL::IB);
        rf_addr = i;
        write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
        itt_idx = uint(MACRO_IDX::VMV);
        src = (i%2) ? OPC_STORAGE::VWR1 : OPC_STORAGE::VWR0;
        imm = gen() % (1 << MACRO_DST_N_BITS);
        dq_data = build_macroinstr(itt_idx, imm, dst, src, sw_len, shift_sa, rf_n,
                             src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
        bank_addr->write(gen() % (1 << BANK_BITS));
        row_addr->write(row_to_write);
        col_addr->write(col_to_write);
        DQ->write(dq_data);
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
    // Write a loop of VMVs
    WR->write(SC_LOGIC_1);
    rf_sel = uint(RF_SEL::LOOP_REG);
    rf_addr = 0;
    write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
    dq_data = loop_to_dq(0, TEST_LOOP_LEN-1, 0xFFFF);
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(row_to_write);
    col_addr->write(col_to_write);
    DQ->write(dq_data);
    wait(CLK_PERIOD, RESOLUTION);
    curCycle++;
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(gen() % (1 << (ROW_BITS-1)));
    col_addr->write(gen() % (1 << COL_BITS));
    DQ->write(gen());
    // RD on, DRAM addresses changing 1/10 probability
    while (curCycle < TESTED_CYCLES) {
        WR->write(SC_LOGIC_0);
        RD->write(SC_LOGIC_1);
        // Change DRAM addresses and DQ with a 1/10 probability
        if (gen() % 10 == 0) {
            bank_addr->write(gen() % (1 << BANK_BITS));
            row_addr->write(gen() % (1 << (ROW_BITS-1)));
            col_addr->write(gen() % (1 << COL_BITS));
            DQ->write(gen());
        }
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
#elif (TESTED_INSTRS == TEST_SHIFT)
    // Write the 3 possible VFUX SHIFTs
    for (i = 0; i < TEST_LOOP_LEN; i++) {
        WR->write(SC_LOGIC_1);
        rf_sel = uint(RF_SEL::IB);
        rf_addr = i;
        write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
        itt_idx = (i % 3) == 0 ? uint(MACRO_IDX::VFUX_SHIFT_FROMVWR) : 
                ((i+1) % 3) == 0 ? uint(MACRO_IDX::VFUX_SHIFT_FROMR3) : 
                            uint(MACRO_IDX::VFUX_SHIFT_FROMTOVWR);
        src = (i%2) ? OPC_STORAGE::VWR1 : OPC_STORAGE::VWR0;
        src0 = (i%2) ? OPC_STORAGE::VWR1 : OPC_STORAGE::VWR0;
        src0_n = gen() % (1 << MACRO_DST_N_BITS);
        shift_sa = i % (SA_MAX_SHIFT+1);
        dst = (i % 4) == 0 ? OPC_STORAGE::R3 : 
                ((i+1) % 4) == 0 ? OPC_STORAGE::R1 :
                ((i+2) % 4) == 0 ? OPC_STORAGE::R2 :
                                OPC_STORAGE::R1R2;
        dst_n = gen() % (1 << MACRO_DST_N_BITS);
        dst_n_sa = gen() % (1 << MACRO_DST_N_SA_BITS);
        sw_len = random_sw_size[gen() % POSSIBLE_SW_SIZES];
        dq_data = build_macroinstr(itt_idx, imm, dst, src, sw_len, shift_sa, rf_n,
                             src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
        bank_addr->write(gen() % (1 << BANK_BITS));
        row_addr->write(row_to_write);
        col_addr->write(col_to_write);
        DQ->write(dq_data);
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
    // Write a loop of VFUX SHIFTs
    WR->write(SC_LOGIC_1);
    rf_sel = uint(RF_SEL::LOOP_REG);
    rf_addr = 0;
    write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
    dq_data = loop_to_dq(0, TEST_LOOP_LEN-1, 0xFFFF);
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(row_to_write);
    col_addr->write(col_to_write);
    DQ->write(dq_data);
    wait(CLK_PERIOD, RESOLUTION);
    curCycle++;
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(gen() % (1 << (ROW_BITS-1)));
    col_addr->write(gen() % (1 << COL_BITS));
    DQ->write(gen());
    // RD on, DRAM addresses changing
    while (curCycle < TESTED_CYCLES) {
        WR->write(SC_LOGIC_0);
        RD->write(SC_LOGIC_1);
        // Change DRAM addresses and DQ with a 1/10 probability
        if (gen() % 10 == 0) {
            bank_addr->write(gen() % (1 << BANK_BITS));
            row_addr->write(gen() % (1 << (ROW_BITS-1)));
            col_addr->write(gen() % (1 << COL_BITS));
            DQ->write(gen());
        }
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
#elif (TESTED_INSTRS == TEST_ADD)
    // Write the 3 possible VFUX ADDs
    for (i = 0; i < TEST_LOOP_LEN; i++) {
        WR->write(SC_LOGIC_1);
        rf_sel = uint(RF_SEL::IB);
        rf_addr = i;
        write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
        itt_idx = (i % 3) == 0 ? uint(MACRO_IDX::VFUX_ADD_FROMVWR) : 
                ((i+1) % 3) == 0 ? uint(MACRO_IDX::VFUX_ADD_FROMR3) : 
                            uint(MACRO_IDX::VFUX_ADD_FROMTOVWR);
        src = (i%2) ? OPC_STORAGE::VWR1 : OPC_STORAGE::VWR0;
        src0 = (i%2) ? OPC_STORAGE::VWR1 : OPC_STORAGE::VWR0;
        src0_n = gen() % (1 << MACRO_DST_N_BITS);
        shift_sa = i % (SA_MAX_SHIFT+1);
        dst = (i % 4) == 0 ? OPC_STORAGE::R3 : 
                ((i+1) % 4) == 0 ? OPC_STORAGE::R1 :
                ((i+2) % 4) == 0 ? OPC_STORAGE::R2 :
                                OPC_STORAGE::R1R2;
        dst_n = gen() % (1 << MACRO_DST_N_BITS);
        dst_n_sa = gen() % (1 << MACRO_DST_N_SA_BITS);
        sw_len = random_sw_size[gen() % POSSIBLE_SW_SIZES];
        dq_data = build_macroinstr(itt_idx, imm, dst, src, sw_len, shift_sa, rf_n,
                             src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
        bank_addr->write(gen() % (1 << BANK_BITS));
        row_addr->write(row_to_write);
        col_addr->write(col_to_write);
        DQ->write(dq_data);
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
    // Write a loop of VFUX ADDs
    WR->write(SC_LOGIC_1);
    rf_sel = uint(RF_SEL::LOOP_REG);
    rf_addr = 0;
    write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
    dq_data = loop_to_dq(0, TEST_LOOP_LEN-1, 0xFFFF);
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(row_to_write);
    col_addr->write(col_to_write);
    DQ->write(dq_data);
    wait(CLK_PERIOD, RESOLUTION);
    curCycle++;
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(gen() % (1 << (ROW_BITS-1)));
    col_addr->write(gen() % (1 << COL_BITS));
    DQ->write(gen());
    // RD on, DRAM addresses changing
    while (curCycle < TESTED_CYCLES) {
        WR->write(SC_LOGIC_0);
        RD->write(SC_LOGIC_1);
        // Change DRAM addresses and DQ with a 1/10 probability
        if (gen() % 10 == 0) {
            bank_addr->write(gen() % (1 << BANK_BITS));
            row_addr->write(gen() % (1 << (ROW_BITS-1)));
            col_addr->write(gen() % (1 << COL_BITS));
            DQ->write(gen());
        }
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
#elif (TESTED_INSTRS == TEST_MUL)
    uint max_mul_len = (TESTED_CSD_LEN / 2) + 2;
    // Write possible MULs
    for (i = 0; i < TEST_LOOP_LEN; i++) {
        WR->write(SC_LOGIC_1);
        rf_sel = uint(RF_SEL::IB);
        rf_addr = i;
        write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
        itt_idx = uint(MACRO_IDX::VFUX_MUL);
        src = (i%2) ? OPC_STORAGE::VWR1 : OPC_STORAGE::VWR0;
        src0 = (i%2) ? OPC_STORAGE::VWR1 : OPC_STORAGE::VWR0;
        src0_n = gen() % (1 << MACRO_DST_N_BITS);
        dst = (i % 4) == 0 ? OPC_STORAGE::R3 : 
                ((i+1) % 4) == 0 ? OPC_STORAGE::R1 :
                ((i+2) % 4) == 0 ? OPC_STORAGE::R2 :
                                OPC_STORAGE::R1R2;
        dst_n = gen() % (1 << MACRO_DST_N_BITS);
        dst_n_sa = gen() % (1 << MACRO_DST_N_SA_BITS);
        sw_len = random_sw_size[gen() % POSSIBLE_SW_SIZES];
        rf_n = i % CSD_ENTRIES;
        dq_data = build_macroinstr(itt_idx, imm, dst, src, sw_len, shift_sa, rf_n,
                             src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
        bank_addr->write(gen() % (1 << BANK_BITS));
        row_addr->write(row_to_write);
        col_addr->write(col_to_write);
        DQ->write(dq_data);
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
    // Write a loop of VFUX MULs
    WR->write(SC_LOGIC_1);
    rf_sel = uint(RF_SEL::LOOP_REG);
    rf_addr = 0;
    write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
    dq_data = loop_to_dq(0, TEST_LOOP_LEN-1, 0xFFFF);
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(row_to_write);
    col_addr->write(col_to_write);
    DQ->write(dq_data);
    wait(CLK_PERIOD, RESOLUTION);
    curCycle++;
    // Write the CSD_LEN
    WR->write(SC_LOGIC_1);
    rf_sel = uint(RF_SEL::CSD_LEN);
    rf_addr = 0;
    write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
    dq_data = TESTED_CSD_LEN;
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(row_to_write);
    col_addr->write(col_to_write);
    DQ->write(dq_data);
    wait(CLK_PERIOD, RESOLUTION);
    curCycle++;
    // Write the CSDRFs
    for (i = 0; i < CSD_ENTRIES; i++) {
        WR->write(SC_LOGIC_1);
        rf_sel = uint(RF_SEL::CSDRF);
        rf_addr = i;
        write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
        dq_data = generate_random_csd(TESTED_CSD_LEN, gen);
        bank_addr->write(gen() % (1 << BANK_BITS));
        row_addr->write(row_to_write);
        col_addr->write(col_to_write);
        DQ->write(dq_data);
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(gen() % (1 << (ROW_BITS-1)));
    col_addr->write(gen() % (1 << COL_BITS));
    DQ->write(gen());
    // RD on, DRAM addresses changing
    while (curCycle < TESTED_CYCLES) {
        WR->write(SC_LOGIC_0);
        RD->write(SC_LOGIC_1);
        // Change DRAM addresses and DQ with a 1/10 probability
        if (gen() % (10) == 0) {
            bank_addr->write(gen() % (1 << BANK_BITS));
            row_addr->write(gen() % (1 << (ROW_BITS-1)));
            col_addr->write(gen() % (1 << COL_BITS));
            DQ->write(gen());
        }
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
        RD->write(SC_LOGIC_0);
        wait(CLK_PERIOD*(max_mul_len-1), RESOLUTION);
        curCycle += max_mul_len - 1;
    }
#elif (TESTED_INSTRS == TEST_PACK)
    // Write the 2 possible PACKs
    for (i = 0; i < TEST_LOOP_LEN; i++) {
        WR->write(SC_LOGIC_1);
        rf_sel = uint(RF_SEL::IB);
        rf_addr = i;
        write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
        itt_idx = (i % 2) ? uint(MACRO_IDX::PACK_TOVWR) : 
                            uint(MACRO_IDX::PACK_TOR3);
        dst = (gen() % 2) ? OPC_STORAGE::VWR1 : OPC_STORAGE::VWR0;
        dst_n = gen() % (1 << MACRO_DST_N_BITS);
        pack_sta = gen() % (1 << MACRO_PACK_STA_BITS);
        repack = random_repack[gen() % POSSIBLE_REPACKS];
        shift_pm = 0;
        dq_data = build_macroinstr(itt_idx, imm, dst, src, sw_len, shift_sa, rf_n,
                             src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
        bank_addr->write(gen() % (1 << BANK_BITS));
        row_addr->write(row_to_write);
        col_addr->write(col_to_write);
        DQ->write(dq_data);
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
    // Write a loop of PACKs
    WR->write(SC_LOGIC_1);
    rf_sel = uint(RF_SEL::LOOP_REG);
    rf_addr = 0;
    write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
    dq_data = loop_to_dq(0, TEST_LOOP_LEN-1, 0xFFFF);
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(row_to_write);
    col_addr->write(col_to_write);
    DQ->write(dq_data);
    wait(CLK_PERIOD, RESOLUTION);
    curCycle++;
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(gen() % (1 << (ROW_BITS-1)));
    col_addr->write(gen() % (1 << COL_BITS));
    DQ->write(gen());
    // RD on, DRAM addresses changing
    while (curCycle < TESTED_CYCLES) {
        WR->write(SC_LOGIC_0);
        RD->write(SC_LOGIC_1);
        // Change DRAM addresses and DQ with a 1/10 probability
        if (gen() % 10 == 0) {
            bank_addr->write(gen() % (1 << BANK_BITS));
            row_addr->write(gen() % (1 << (ROW_BITS-1)));
            col_addr->write(gen() % (1 << COL_BITS));
            DQ->write(gen());
        }
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
#elif (TESTED_INSTRS == TEST_EXIT)
    // Write one EXIT
    WR->write(SC_LOGIC_1);
    rf_sel = uint(RF_SEL::IB);
    rf_addr = 0;
    write_RF_row_col (row_to_write, col_to_write, rf_sel, rf_addr);
    itt_idx = uint(MACRO_IDX::EXIT);
    imm = 0;
    dq_data = build_macroinstr(itt_idx, imm, dst, src, sw_len, shift_sa, rf_n,
                             src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(row_to_write);
    col_addr->write(col_to_write);
    DQ->write(dq_data);
    wait(CLK_PERIOD, RESOLUTION);
    curCycle++;
    bank_addr->write(gen() % (1 << BANK_BITS));
    row_addr->write(gen() % (1 << (ROW_BITS-1)));
    col_addr->write(gen() % (1 << COL_BITS));
    DQ->write(gen());
    // RD on, DRAM addresses changing
    while (curCycle < TESTED_CYCLES) {
        WR->write(SC_LOGIC_0);
        RD->write(SC_LOGIC_1);
        // Change DRAM addresses and DQ with a 1/10 probability
        if (gen() % 10 == 0) {
            bank_addr->write(gen() % (1 << BANK_BITS));
            row_addr->write(gen() % (1 << (ROW_BITS-1)));
            col_addr->write(gen() % (1 << COL_BITS));
            DQ->write(gen());
        }
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }
#else
    cout << "No valid test selected" << endl;
    sc_stop();
#endif

    cout << "Simulation finished at cycle " << dec << curCycle << endl;

    // Stop simulation
    sc_stop();
}

void cp_driver::write_RF_row_col(sc_uint<ROW_BITS> &row_addr, sc_uint<COL_BITS> &col_addr,
                        sc_uint<RF_SEL_BITS> &rf_sel, sc_uint<RF_ADDR_BITS> &rf_addr) {
    sc_uint<ROW_BITS + COL_BITS> rowcol_addr(0);
    rowcol_addr.range(RF_SEL_BITS + RF_ADDR_BITS - 1, RF_ADDR_BITS) = rf_sel;
    rowcol_addr.range(RF_ADDR_BITS - 1, 0) = rf_addr;
    rowcol_addr.range(ROW_BITS + COL_BITS - 1, ROW_BITS + COL_BITS - 1) = 1;
    row_addr = rowcol_addr.range(ROW_BITS + COL_BITS - 1, COL_BITS);
    col_addr = rowcol_addr.range(COL_BITS - 1, 0);
    return;
}

uint64_t cp_driver::loop_to_dq(uint64_t start, uint64_t end, uint64_t iter) {
    uint64_t dq_data = 0;
    dq_data |= (start & 0xFFFF) << 32;
    dq_data |= (end & 0xFFFF) << 16;
    dq_data |= (iter & 0xFFFF);
    return dq_data;
}

uint64_t cp_driver::generate_random_csd(uint csd_len, std::mt19937_64 &gen) {
    std::uniform_int_distribution<uint64_t> dis(0, 2);
    uint64_t csd = 0;
    uint64_t new_bit = 0;
    uint64_t prev_bit = 0;
    for (uint i = 0; i < csd_len; i++) {
        if (prev_bit == 0) {
            new_bit = dis(gen);
        
        } else {
            new_bit = 0;
        }
        prev_bit = new_bit;    
        csd |= new_bit << 2*i;
    }
    return csd;
}

#endif // MIXED_SIM
