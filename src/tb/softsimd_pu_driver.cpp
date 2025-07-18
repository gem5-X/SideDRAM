#include "softsimd_pu_driver.h"
#include <random>
#include <cmath>
#include "../cnm_base.h"

using half_float::half;

void softsimd_pu_driver::driver_thread() { // TODO this is not currently functional

    uint clk_period = CLK_PERIOD;
    uint64_t i, j, k;
    uint64_t DQ_tosend = 0;
    sc_lv<DQ_BITS> DQ_aux;
    sc_lv<VWR_BITS> vwr_tosend;
#if VWR_DRAM_CLK == 1
    sc_bigint<DRAM_BITS> bank_tosend = 0;
#else
    sc_lv<DRAM_BITS> bank_tosend = 0;
#endif
    sc_lv<DRAM_BITS> allzs(SC_LOGIC_Z);
    sc_uint<ROW_BITS> row_aux = 0;

    // Randomization tools
    std::mt19937 gen(11111);    // Standard mersenne_twister_engine seeded
    std::uniform_int_distribution<> dis_DQ(0, 10000000);

    // Instruction word
    uint32_t instruction;
    MACRO_IDX itt_idx;
    uint imm, rf_n, src0_n, pack_sta, shift_sa, shift_pm, dst_n, dst_n_sa;
    OPC_STORAGE dst, src, src0;
    SWSIZE sw_len;
    SWREPACK repack;

    shift_sa = 0;

    cout << "Simulation start" << endl;

    // Initialization
    rst->write(false);
    RD->write(false);
    WR->write(false);
    ACT->write(false);
    AB_mode->write(false);
    pim_mode->write(false);
    bank_addr->write(0);
    row_addr->write(row_aux);
    col_addr->write(0);
    DQ->write(0);
#if DUAL_BANK_INTERFACE
    even_bus->write(allzs);
    odd_bus->write(allzs);
#else
    dram_bus->write(allzs);
#endif

    wait(clk_period / 2, RESOLUTION);
    rst->write(1);
    wait(0, RESOLUTION);

    wait(clk_period / 2 + 100, RESOLUTION);

    cout << "Inputs initialized" << endl;
//    wait(clk_period, RESOLUTION);

#if VWR_DRAM_CLK == 1

    /// ---------------------------------------------------------------------
    ///  --- Test of RLB, WLB, VMV, RMV, ADD (TO ZERO), GLMV, NOP and EXIT ---
    ///     1x  RLB VWR0 <- ADDR
    ///     1x  RLB VWR1 <- ADDR
    ///     1x  NOP (4 CLK)
    ///     1x  GLMV VWR1 IDX=4
    ///     1x  VMV VWR0_0 -> R0
    ///     1x  ADD R0 + ZERO -> R3
    ///     1x  RMV R3 -> VWR2_3
    ///     1x  VMV VWR0_1 -> R0
    ///     1x  ADD R0 + ZERO -> R3
    ///     1x  RMV R3 -> VWR2_2
    ///     1x  VMV VWR0_2 -> R0
    ///     1x  ADD R0 + ZERO -> R3
    ///     1x  RMV R3 -> VWR2_1
    ///     1x  VMV VWR0_3 -> R0
    ///     1x  ADD R0 + ZERO -> R3
    ///     1x  RMV R3 -> VWR2_0
    ///     1x  WLB VWR1 -> ADDR
    ///     1x  WLB VWR2 -> ADDR
    ///     1x  EXIT

    // Fill the VWRs and RFs
    wait(0, RESOLUTION);

    // Write the instructions to the CRF
    ///     1x  RLB VWR0 <- ADDR
    itt_idx = MACRO_IDX::RLB;
    imm = (1 << WORDS_PER_VWR) - 1; // Mask
    dst = OPC_STORAGE::VWR0;
    src = OPC_STORAGE::DRAM;
    sw_len = SWSIZE::INV;
    rf_n = 0;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(0);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  RLB VWR1 <- ADDR
    itt_idx = MACRO_IDX::RLB;
    imm = (1 << (WORDS_PER_VWR-1)) - 1; // Mask (Don't store most significant word)
    dst = OPC_STORAGE::VWR1;
    src = OPC_STORAGE::DRAM;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(1);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  NOP (4 CLK)
    itt_idx = MACRO_IDX::NOP;
    imm = 4; // NOP Cycles
    dst = OPC_STORAGE::IB;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(2);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  GLMV VWR1 IDX=4
    itt_idx = MACRO_IDX::GLMV;
    imm = 4; // GLMV idx_start
    dst = OPC_STORAGE::VWR1;
    src = OPC_STORAGE::VWR1;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(3);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  VMV VWR0_0 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 0; // VWR0 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR0;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(4);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  ADD R0 + ZERO -> R3
    itt_idx = MACRO_IDX::VFUX_ADD_FROMR3;
    imm = 0; // VWR0 index
    dst = OPC_STORAGE::R3;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::ZERO;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(5);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  RMV R3 -> VWR2_3
    itt_idx = MACRO_IDX::RMV;
    imm = 3; // VWR2 index
    dst = OPC_STORAGE::VWR2;
    src = OPC_STORAGE::R3;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(6);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  VMV VWR0_1 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 1; // VWR0 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR0;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(7);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  ADD R0 + ZERO -> R3
    itt_idx = MACRO_IDX::VFUX_ADD_FROMR3;
    imm = 0; // VWR0 index
    dst = OPC_STORAGE::R3;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::ZERO;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(8);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  RMV R3 -> VWR2_2
    itt_idx = MACRO_IDX::RMV;
    imm = 2; // VWR2 index
    dst = OPC_STORAGE::VWR2;
    src = OPC_STORAGE::R3;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(9);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  VMV VWR0_2 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 2; // VWR0 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR0;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(10);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  ADD R0 + ZERO -> R3
    itt_idx = MACRO_IDX::VFUX_ADD_FROMR3;
    imm = 0; // VWR0 index
    dst = OPC_STORAGE::R3;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::ZERO;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(11);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  RMV R3 -> VWR2_1
    itt_idx = MACRO_IDX::RMV;
    imm = 1; // VWR2 index
    dst = OPC_STORAGE::VWR2;
    src = OPC_STORAGE::R3;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(12);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  VMV VWR0_3 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 3; // VWR0 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR0;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(13);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  ADD R0 + ZERO -> R3
    itt_idx = MACRO_IDX::VFUX_ADD_FROMR3;
    imm = 0; // VWR0 index
    dst = OPC_STORAGE::R3;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::ZERO;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(14);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  RMV R3 -> VWR2_0
    itt_idx = MACRO_IDX::RMV;
    imm = 0; // VWR2 index
    dst = OPC_STORAGE::VWR2;
    src = OPC_STORAGE::R3;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(15);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  WLB VWR1 -> ADDR
    itt_idx = MACRO_IDX::WLB;
    imm = (1 << WORDS_PER_VWR) - 1; // Mask (Don't store most significant word)
    dst = OPC_STORAGE::DRAM;
    src = OPC_STORAGE::VWR1;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(16);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  WLB VWR2 -> ADDR
    itt_idx = MACRO_IDX::WLB;
    imm = (1 << WORDS_PER_VWR) - 1; // Mask (Don't store most significant word)
    dst = OPC_STORAGE::DRAM;
    src = OPC_STORAGE::VWR2;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(17);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  EXIT
    itt_idx = MACRO_IDX::EXIT;
    imm = 0;
    dst = OPC_STORAGE::IB;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(18);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);


    // Execute
    WR->write(false);
    RD->write(true);
    AB_mode->write(false);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = false;  // Executing PIM
    row_addr->write(row_aux);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    RD->write(false);
    for (j = 0; j < DRAM_BITS/WORD_BITS; j++) {
        switch (j) {
            case 0: DQ_tosend = 0xAAAAAAAAAAAAAAAA; break;
            case 1: DQ_tosend = 0xBBBBBBBBBBBBBBBB; break;
            case 2: DQ_tosend = 0xCCCCCCCCCCCCCCCC; break;
            case 3: DQ_tosend = 0xDDDDDDDDDDDDDDDD; break;
            default: DQ_tosend = 0xEEEEEEEEEEEEEEEE; break;
        }
        bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
    }
    dram_bus->write(bank_tosend);   // One cycle later
    wait(1* clk_period, RESOLUTION);
    RD->write(true);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    RD->write(false);
    for (j = 0; j < DRAM_BITS/WORD_BITS; j++) {
        DQ_tosend = 0xCCCCCCCCCCCCCCCC;
//        for (k = 0; k < WORD_BITS/12; k++) {
//            DQ_tosend |= (((-(j+k+48)) & 0x7FF) | (j ? 0 : 0x800)) << (k * 12);
//        }
        bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
    }
    dram_bus->write(bank_tosend);   // One cycle later
    wait(1* clk_period, RESOLUTION);
    RD->write(true);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    dram_bus->write(allzs);
    wait(3* clk_period, RESOLUTION); // Test NOP
    for (j = 0; j < 13; j++) {
        RD->write(true);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
        RD->write(false);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
    }
    for (j = 0; j < 2; j++) {   // WLBs
        WR->write(true);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
        WR->write(false);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
    }
    RD->write(true);    // EXIT
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    RD->write(false);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);

    pim_mode->write(false);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);


    /// ---------------------------------------------------------------------
    /// --- Test of PERM and PACK, with HW LOOP ---
    ///     1x  RLB VWR0 <- ADDR
    ///     1x  RLB VWR1 <- ADDR
    ///     --- HW LOOP START --- (2x)
    ///     1x  RLB VWR2 <- ADDR
    ///     1x  VMV VWR2_0 -> R0
    ///     1x  ADD R0 + ZERO -> R1
    ///     1x  VMV VWR2_1 -> R0
    ///     1x  ADD R0 + ZERO -> R2
    ///     1x  PERM B8 IN_LOC_2 OUT_SEL_BOTH VWR3_0
    ///     1x  RMV R3 -> VWR3_1
    ///     1x  WLB VWR3 -> ADDR
    ///     --- HW LOOP END ---   (2x)
    ///     1x  VMV VWR0_0 -> R0
    ///     1x  ADD R0 + ZERO -> R1
    ///     1x  VMV VWR1_0 -> R0
    ///     1x  ADD R0 + ZERO -> R2
    ///     4x  PACK SW_LEN_CHANGE_x IN_LOC_x OUT_VWR2_x
    ///     1x  VMV VWR0_1 -> R0
    ///     1x  ADD R0 + ZERO -> R1
    ///     1x  VMV VWR1_1 -> R0
    ///     1x  ADD R0 + ZERO -> R2
    ///     4x  PACK SW_LEN_CHANGE_x IN_LOC_x OUT_VWR3_x
    ///     1x  WLB VWR2 -> ADDR
    ///     1x  WLB VWR3 -> ADDR
    ///     1X EXIT

    // Fill the GRFs and SRF
    wait(0, RESOLUTION);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::LOOP_REG); // Filling VWR
    row_addr->write(row_aux);
    col_addr->write(0);
    vwr_tosend = 0; // Prepare VWR to send via DQs
    DQ_tosend = 0;
    DQ_tosend |= (uint64_t) (2 & 0xFFFF) << 32; // Start address
    DQ_tosend |= (uint64_t) (9 & 0xFFFF) << 16; // End address
    DQ_tosend |= (uint64_t) (2 & 0xFFFF);       // Number of iterations
    DQ->write(DQ_tosend);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);


    // Write the instructions to the CRF
    ///     1x  RLB VWR0 <- ADDR
    itt_idx = MACRO_IDX::RLB;
    imm = (1 << WORDS_PER_VWR) - 1; // Mask
    dst = OPC_STORAGE::VWR0;
    src = OPC_STORAGE::DRAM;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(0);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  RLB VWR1 <- ADDR
    itt_idx = MACRO_IDX::RLB;
    imm = (1 << WORDS_PER_VWR) - 1; // Mask
    dst = OPC_STORAGE::VWR1;
    src = OPC_STORAGE::DRAM;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(1);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  RLB VWR2 <- ADDR
    itt_idx = MACRO_IDX::RLB;
    imm = 0b11;         // Mask (Only 2 rightmost words)
    dst = OPC_STORAGE::VWR2;
    src = OPC_STORAGE::DRAM;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(2);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  VMV VWR2_0 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 0;                // VWR2 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR2;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(3);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  ADD R0 + ZERO -> R1
    itt_idx = MACRO_IDX::VFUX_ADD_FROMR3;
    imm = 0;
    dst = OPC_STORAGE::R1;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::ZERO;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(4);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  VMV VWR2_1 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 1;                // VWR2 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR2;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(5);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  ADD R0 + ZERO -> R2
    itt_idx = MACRO_IDX::VFUX_ADD_FROMR3;
    imm = 0;
    dst = OPC_STORAGE::R2;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::ZERO;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(6);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  PERM B8 IN_LOC_2 OUT_SEL_BOTH VWR3_0
    itt_idx = MACRO_IDX::PERM_TOBOTH;
    imm = 0;
    dst = OPC_STORAGE::VWR3;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::B8_B8;
    pack_sta = 2;
    shift_pm = 0;
    dst_n = 0;  // VWR3 index
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(7);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  RMV R3 -> VWR3_1
    itt_idx = MACRO_IDX::RMV;
    imm = 1; // VWR3 index
    dst = OPC_STORAGE::VWR3;
    src = OPC_STORAGE::R3;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(8);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  WLB VWR3 -> ADDR
    itt_idx = MACRO_IDX::WLB;
    imm = 0b11;         // Mask (Only 2 rightmost words)
    dst = OPC_STORAGE::DRAM;
    src = OPC_STORAGE::VWR3;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(9);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  VMV VWR0_0 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 0;                // VWR0 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR0;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(10);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  ADD R0 + ZERO -> R1
    itt_idx = MACRO_IDX::VFUX_ADD_FROMR3;
    imm = 0;
    dst = OPC_STORAGE::R1;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::ZERO;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(11);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  VMV VWR1_0 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 0;                // VWR1 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR1;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(12);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  ADD R0 + ZERO -> R2
    itt_idx = MACRO_IDX::VFUX_ADD_FROMR3;
    imm = 0;
    dst = OPC_STORAGE::R2;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::ZERO;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(13);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     4x  PACK SW_LEN_CHANGE_x IN_LOC_x OUT_VWR2_x
    for (i = 0; i < 4; i++) {
        itt_idx = MACRO_IDX::PACK_TOVWR;
        imm = 0;
        dst = OPC_STORAGE::VWR2;
        src = OPC_STORAGE::IB;
        sw_len = SWSIZE::INV;
        src0_n = 0;
        src0 = OPC_STORAGE::IB;
        switch (i) {
//            case 0: repack = SWREPACK::B12_B16; break;
//            case 1: repack = SWREPACK::B12_B24; break;
//            case 2: repack = SWREPACK::B12_B8;  break;
//            case 3: repack = SWREPACK::B12_B6;  break;
            case 0: repack = SWREPACK::B12_B16; break;
            case 1: repack = SWREPACK::B12_B16; break;
            case 2: repack = SWREPACK::B12_B8;  break;
            case 3: repack = SWREPACK::B12_B8;  break;
        }
        pack_sta = i;
        shift_pm = 0;
        dst_n = i;  // VWR2 index
        dst_n_sa = 0;
        instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
        WR->write(true);
        AB_mode->write(true);
        pim_mode->write(true);
        bank_addr->write(1);
        row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
        row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
        row_addr->write(row_aux);
        col_addr->write(14+i);
        DQ->write(instruction);
        wait(clk_period, RESOLUTION);
        wait(0, RESOLUTION);
    }

    ///     1x  VMV VWR0_1 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 1;                // VWR0 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR0;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(18);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  ADD R0 + ZERO -> R1
    itt_idx = MACRO_IDX::VFUX_ADD_FROMR3;
    imm = 0;
    dst = OPC_STORAGE::R1;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::ZERO;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(19);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  VMV VWR1_1 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 1;                // VWR1 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR1;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(20);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  ADD R0 + ZERO -> R2
    itt_idx = MACRO_IDX::VFUX_ADD_FROMR3;
    imm = 0;
    dst = OPC_STORAGE::R2;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::ZERO;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(21);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     4x  PACK SW_LEN_CHANGE_x IN_LOC_x OUT_VWR3_x
    for (i = 0; i < 4; i++) {
        itt_idx = MACRO_IDX::PACK_TOVWR;
        imm = 0;
        dst = OPC_STORAGE::VWR3;
        src = OPC_STORAGE::IB;
        sw_len = SWSIZE::INV;
        src0_n = 0;
        src0 = OPC_STORAGE::IB;
        switch (i) {
//            case 0: repack = SWREPACK::B6_B3;   break;
//            case 1: repack = SWREPACK::B6_B4;   break;
//            case 2: repack = SWREPACK::B6_B8;   break;
//            case 3: repack = SWREPACK::B6_B12;  break;
            case 0: repack = SWREPACK::B6_B4;   break;
            case 1: repack = SWREPACK::B6_B4;   break;
            case 2: repack = SWREPACK::B6_B8;   break;
            case 3: repack = SWREPACK::B6_B8;  break;
        }
        pack_sta = i;
        shift_pm = 0;
        dst_n = i;  // VWR3 index
        dst_n_sa = 0;
        instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
        WR->write(true);
        AB_mode->write(true);
        pim_mode->write(true);
        bank_addr->write(1);
        row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
        row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
        row_addr->write(row_aux);
        col_addr->write(22+i);
        DQ->write(instruction);
        wait(clk_period, RESOLUTION);
        wait(0, RESOLUTION);
    }

    ///     1x  WLB VWR2 -> ADDR
    itt_idx = MACRO_IDX::WLB;
    imm = (1 << WORDS_PER_VWR) - 1; // Mask
    dst = OPC_STORAGE::DRAM;
    src = OPC_STORAGE::VWR2;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(26);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  WLB VWR3 -> ADDR
    itt_idx = MACRO_IDX::WLB;
    imm = (1 << WORDS_PER_VWR) - 1; // Mask
    dst = OPC_STORAGE::DRAM;
    src = OPC_STORAGE::VWR3;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(27);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  EXIT
    itt_idx = MACRO_IDX::EXIT;
    imm = 0;
    dst = OPC_STORAGE::IB;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(28);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);


    // Execute
    WR->write(false);
    RD->write(true);
    AB_mode->write(false);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = false;  // Executing PIM
    row_addr->write(row_aux);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    RD->write(false);
    for (j = 0; j < DRAM_BITS/WORD_BITS; j++) { // To VWR0
        DQ_tosend = 0;
        switch (j) {
            case 0:
                for (k = 0; k < WORD_BITS/12; k++)
                    DQ_tosend |= (k*100 & 0xFFF) << (k * 12);
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
            break;
            case 1:
                for (k = 0; k < WORD_BITS/6; k++)
                    DQ_tosend |= (k*2 & 0x3F) << (k * 6);
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
            break;
            default:
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = 0;
            break;
        }
    }
    dram_bus->write(bank_tosend);   // One cycle later
    wait(1* clk_period, RESOLUTION);
    RD->write(true);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    RD->write(false);
    for (j = 0; j < DRAM_BITS/WORD_BITS; j++) { // To VWR1
        DQ_tosend = 0;
        switch (j) {
            case 0:
                for (k = 0; k < WORD_BITS/12; k++)
                    DQ_tosend |= ((-k)*100 & 0xFFF) << (k * 12);
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
            break;
            case 1:
                for (k = 0; k < WORD_BITS/6; k++)
                    DQ_tosend |= ((-k)*2 & 0x3F) << (k * 6);
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
            break;
            default:
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = 0;
            break;
        }
    }
    dram_bus->write(bank_tosend);   // One cycle later
    wait(1* clk_period, RESOLUTION);
    RD->write(true);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    RD->write(false);
    for (j = 0; j < DRAM_BITS/WORD_BITS; j++) { // To VWR2
        DQ_tosend = 0;
        switch (j) {
            case 0:
                for (k = 0; k < WORD_BITS/8; k++)
                    DQ_tosend |= (k & 0xFF) << (k * 8);
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
            break;
            case 1:
                for (k = 0; k < WORD_BITS/8; k++)
                    DQ_tosend |= ((k + WORD_BITS/8) & 0xFF) << (k * 8);
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
            break;
            default:
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = 0;
            break;
        }
    }
    dram_bus->write(bank_tosend);   // One cycle later
    wait(1* clk_period, RESOLUTION);
    for (j = 0; j < 6; j++) {
        RD->write(true);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
        RD->write(false);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
    }
    WR->write(true);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    WR->write(false);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    // Second HW LOOP iteration
    RD->write(true);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    RD->write(false);
    for (j = 0; j < DRAM_BITS/WORD_BITS; j++) { // To VWR2
        DQ_tosend = 0;
        switch (j) {
            case 0:
                for (k = 0; k < WORD_BITS/8; k++)
                    DQ_tosend |= ((-k) & 0xFF) << (k * 8);
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
            break;
            case 1:
                for (k = 0; k < WORD_BITS/8; k++)
                    DQ_tosend |= ((-(k + WORD_BITS/8)) & 0xFF) << (k * 8);
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
            break;
            default:
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = 0;
            break;
        }
    }
    dram_bus->write(bank_tosend);   // One cycle later
    wait(1* clk_period, RESOLUTION);
    for (j = 0; j < 6; j++) {
        RD->write(true);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
        RD->write(false);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
    }
    WR->write(true);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    WR->write(false);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    // After HW LOOP
    for (j = 0; j < 16; j++) {
        RD->write(true);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
        RD->write(false);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
    }
    for (j = 0; j < 2; j++) {   // WLBs
        WR->write(true);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
        WR->write(false);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
    }
    RD->write(true);    // EXIT
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    RD->write(false);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);

    pim_mode->write(false);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);


    /// ---------------------------------------------------------------------
    /// --- Test of ADD, SUB and MUL ---
    ///     1x  RLB VWR0 <- ADDR
    ///     1x  RLB VWR1 <- ADDR
    ///     1x  VMV VWR0_1 -> R0
    ///     1x  ADD R0 + VWR0_0 -> R3
    ///     --- HW LOOP START --- (9x)
    ///     1x  ADD R0 + R3 -> R3
    ///     --- HW LOOP END ---   (9x)
    ///     1x  RMV R3 -> VWR2_0
    ///     1x  VMV VWR1_0 -> R0
    ///     1x  SUB VWR1_1 - R0 -> R3
    ///     1x  RMV R3 -> VWR2_1
    ///     1x  VMV VWR1_2 -> R0
    ///     1x  SUB VWR1_3 - R0 -> R3
    ///     1x  RMV R3 -> VWR2_2
    ///     1x  SUB VWR1_4 - R0 -> R3
    ///     1x  RMV R3 -> VWR2_3
    ///     1x  VMV VWR0_1 -> R0
    ///     1x  MUL CSD0 -> R3
    ///     1x  RMV R3 -> VWR3_0
    ///     1x  VMV VWR0_2 -> R0
    ///     1x  MUL CSD1 -> R1R2
    ///     1x  PERM IN_LOC_0 OUT_SEL_VWR VWR3_1
    ///     1x  WLB VWR2 -> ADDR
    ///     1x  WLB VWR3 -> ADDR
    ///     1X EXIT


    // Fill the GRFs and SRF
    wait(0, RESOLUTION);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::LOOP_REG); // Filling VWR
    row_addr->write(row_aux);
    col_addr->write(0);
    vwr_tosend = 0; // Prepare VWR to send via DQs
    DQ_tosend = 0;
    DQ_tosend |= (uint64_t) (4 & 0xFFFF) << 32; // Start address
    DQ_tosend |= (uint64_t) (4 & 0xFFFF) << 16; // End address
    DQ_tosend |= (uint64_t) (9 & 0xFFFF);       // Number of iterations
    DQ->write(DQ_tosend);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::CSD_LEN); // Filling VWR
    row_addr->write(row_aux);
    col_addr->write(0);
    DQ_tosend = 10; // Length of CSD multiplicands
    DQ->write(DQ_tosend);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::CSDRF); // Filling VWR
    row_addr->write(row_aux);
    col_addr->write(0);
    DQ_tosend = 0b10000000010001000000;   // -000+0+000
    DQ->write(DQ_tosend);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::CSDRF); // Filling VWR
    row_addr->write(row_aux);
    col_addr->write(1);
    DQ_tosend = 0b01001000010010;   // +0-0+0-
    DQ->write(DQ_tosend);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);


    // Write the instructions to the CRF
    ///     1x  RLB VWR0 <- ADDR
    itt_idx = MACRO_IDX::RLB;
    imm = (1 << WORDS_PER_VWR) - 1; // Mask
    dst = OPC_STORAGE::VWR0;
    src = OPC_STORAGE::DRAM;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(0);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  RLB VWR1 <- ADDR
    itt_idx = MACRO_IDX::RLB;
    imm = (1 << WORDS_PER_VWR) - 1; // Mask
    dst = OPC_STORAGE::VWR1;
    src = OPC_STORAGE::DRAM;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(1);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  VMV VWR0_1 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 1;                // VWR0 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR0;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(2);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  ADD R0 + VWR0_0 -> R3
    itt_idx = MACRO_IDX::VFUX_ADD_FROMVWR;
    imm = 0;
    dst = OPC_STORAGE::R3;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::B12;
    src0_n = 0;
    src0 = OPC_STORAGE::VWR0;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(3);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  ADD R0 + R3 -> R3
    itt_idx = MACRO_IDX::VFUX_ADD_FROMR3;
    imm = 0;
    dst = OPC_STORAGE::R3;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::B12;
    src0_n = 0;
    src0 = OPC_STORAGE::R3;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(4);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  RMV R3 -> VWR2_0
    itt_idx = MACRO_IDX::RMV;
    imm = 0;    // VWR2 index
    dst = OPC_STORAGE::VWR2;
    src = OPC_STORAGE::R3;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(5);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  VMV VWR1_0 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 0;                // VWR1 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR1;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(6);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  SUB VWR1_1 - R0 -> R3
    itt_idx = MACRO_IDX::VFUX_SUB_FROMVWR;
    imm = 0;
    dst = OPC_STORAGE::R3;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::B8;
    src0_n = 1;
    src0 = OPC_STORAGE::VWR1;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(7);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  RMV R3 -> VWR2_1
    itt_idx = MACRO_IDX::RMV;
    imm = 1;    // VWR2 index
    dst = OPC_STORAGE::VWR2;
    src = OPC_STORAGE::R3;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(8);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  VMV VWR1_2 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 2;                // VWR1 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR1;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(9);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  SUB VWR1_3 - R0 -> R3
    itt_idx = MACRO_IDX::VFUX_SUB_FROMVWR;
    imm = 0;
    dst = OPC_STORAGE::R3;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::B8;
    src0_n = 3;
    src0 = OPC_STORAGE::VWR1;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(10);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  RMV R3 -> VWR2_2
    itt_idx = MACRO_IDX::RMV;
    imm = 2;    // VWR2 index
    dst = OPC_STORAGE::VWR2;
    src = OPC_STORAGE::R3;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(11);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  SUB VWR1_4 - R0 -> R3
    itt_idx = MACRO_IDX::VFUX_SUB_FROMVWR;
    imm = 0;
    dst = OPC_STORAGE::R3;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::B8;
    src0_n = 4;
    src0 = OPC_STORAGE::VWR1;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(12);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  RMV R3 -> VWR2_3
    itt_idx = MACRO_IDX::RMV;
    imm = 3;    // VWR2 index
    dst = OPC_STORAGE::VWR2;
    src = OPC_STORAGE::R3;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(13);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  VMV VWR0_1 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 1;                // VWR0 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR0;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(14);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  MUL CSD0 -> R3
    itt_idx = MACRO_IDX::VFUX_MUL;
    imm = 0;
    dst = OPC_STORAGE::R3;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::B8;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(15);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  RMV R3 -> VWR3_0
    itt_idx = MACRO_IDX::RMV;
    imm = 0;    // VWR3 index
    dst = OPC_STORAGE::VWR3;
    src = OPC_STORAGE::R3;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(16);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  VMV VWR0_2 -> R0
    itt_idx = MACRO_IDX::VMV;
    imm = 2;                // VWR0 index
    dst = OPC_STORAGE::R0;
    src = OPC_STORAGE::VWR0;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(0);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(17);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  MUL CSD1 -> R1R2
    itt_idx = MACRO_IDX::VFUX_MUL;
    imm = 0;
    dst = OPC_STORAGE::R1R2;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::B8;
    src0_n = 1;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(18);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  PERM IN_LOC_0 OUT_SEL_VWR VWR3_1
    itt_idx = MACRO_IDX::PERM_TOVWR;
    imm = 0;
    dst = OPC_STORAGE::VWR3;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::B8_B8;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 1;  // VWR3 index
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(19);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  WLB VWR2 -> ADDR
    itt_idx = MACRO_IDX::WLB;
    imm = (1 << WORDS_PER_VWR) - 1; // Mask
    dst = OPC_STORAGE::DRAM;
    src = OPC_STORAGE::VWR2;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(20);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  WLB VWR3 -> ADDR
    itt_idx = MACRO_IDX::WLB;
    imm = (1 << WORDS_PER_VWR) - 1; // Mask
    dst = OPC_STORAGE::DRAM;
    src = OPC_STORAGE::VWR3;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(21);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    ///     1x  EXIT
    itt_idx = MACRO_IDX::EXIT;
    imm = 0;
    dst = OPC_STORAGE::IB;
    src = OPC_STORAGE::IB;
    sw_len = SWSIZE::INV;
    src0_n = 0;
    src0 = OPC_STORAGE::IB;
    repack = SWREPACK::INV;
    pack_sta = 0;
    shift_pm = 0;
    dst_n = 0;
    dst_n_sa = 0;
    instruction = build_macroinstr(uint(itt_idx), imm, dst, src, sw_len, shift_sa, rf_n, src0_n, src0, repack, pack_sta, shift_pm, dst_n, dst_n_sa);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    bank_addr->write(1);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = uint(RF_SEL::IB);
    row_addr->write(row_aux);
    col_addr->write(22);
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);


    // Execute
    WR->write(false);
    RD->write(true);
    AB_mode->write(false);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = false;  // Executing PIM
    row_addr->write(row_aux);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    RD->write(false);
    for (j = 0; j < DRAM_BITS/WORD_BITS; j++) { // To VWR0
        DQ_tosend = 0;
        switch (j) {
            case 0:
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = 0;
            break;
            case 1:
                for (k = 0; k < WORD_BITS/12; k++)
                    DQ_tosend |= (10*k & 0xFF) << (k * 12);
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
            break;
            case 2:
                for (k = 0; k < WORD_BITS/8; k++)
                    DQ_tosend |= (((-1)^k)*27*k & 0x7F) << (k * 8);
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
            break;
            default:
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = 0;
            break;
        }
    }
    dram_bus->write(bank_tosend);   // One cycle later
    wait(1* clk_period, RESOLUTION);
    RD->write(true);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    RD->write(false);
    for (j = 0; j < DRAM_BITS/WORD_BITS; j++) { // To VWR1
        DQ_tosend = 0;
        switch (j) {
            case 0:
            case 1:
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = 0x7F0F43707F7F;
            break;
            case 2:
                for (k = 0; k < WORD_BITS/8; k++)
                    DQ_tosend |= (k*5 & 0x7F) << (k * 8);
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
            break;
            case 3:
                for (k = 0; k < WORD_BITS/8; k++)
                    DQ_tosend |= (((-1)^k)*10*k & 0x7F) << (k * 8);
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
            break;
            case 4:
                for (k = 0; k < WORD_BITS/8; k++)
                    DQ_tosend |= ((-k)*5 & 0xFF) << (k * 8);
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = DQ_tosend;
            break;
            default:
                bank_tosend.range((j+1)*WORD_BITS-1, j*WORD_BITS) = 0;
            break;
        }
    }
    dram_bus->write(bank_tosend);   // One cycle later
    wait(1* clk_period, RESOLUTION);
    for (j = 0; j < 21; j++) {
        RD->write(true);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
        RD->write(false);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
    }
    RD->write(true);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    RD->write(false);
    dram_bus->write(allzs);
    wait(5* clk_period, RESOLUTION);    // First MUL
    for (j = 0; j < 2; j++) {
        RD->write(true);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
        RD->write(false);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
    }
    RD->write(true);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    RD->write(false);
    dram_bus->write(allzs);
    wait(5* clk_period, RESOLUTION);    // Second MUL
    RD->write(true);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    RD->write(false);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    for (j = 0; j < 2; j++) {   // WLBs
        WR->write(true);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
        WR->write(false);
        dram_bus->write(allzs);
        wait(1* clk_period, RESOLUTION);
    }
    RD->write(true);    // EXIT
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);
    RD->write(false);
    dram_bus->write(allzs);
    wait(1* clk_period, RESOLUTION);

    pim_mode->write(false);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);


#else   // i.e. VWR_DRAM_CLK > 1

    /// ---------------------------------------------------------------------
    ///  --- Test of HW LOOP and Multi-cycle interface ---
    ///     4x  MOV VWR0 <- BANK
    ///     4x  MOV VWR1 <- BANK
    ///     --- Start Loop ---
    ///     10x ADD VWR1_x <- VWR0_x + VWR1_x
    ///     10x NOP
    ///     --- End Loop ---
    ///     1x  MOV BANKS <- VWR1x
    ///     1x  EXIT

    // Write the instructions to the CRF
    for (i = 0; i < VWR_DRAM_CLK; i++) {
        MOV_BIT = true;
        SA_BIT = false;
        PM_BIT = false;
        OP_REST = MOP_MOV;
        DST = OPC_VWR_0;
    #if DUAL_BANK_INTERFACE
        SRC0 = OPC_EVENBANK;
    #else
        SRC0 = OPC_DRAM;
    #endif
        WR->write(true);
        AB_mode->write(true);
        pim_mode->write(true);
        row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
        row_aux.range(RF_SEL_BITS - 1, 0) = RF_IB;
        row_addr->write(row_aux);
        col_addr->write(2*i);
        instruction = build_instr(MOV_BIT, SA_BIT, PM_BIT, OP_ADD.to_uint(), OP_MASK.to_uint(), OP_REST.to_uint(),
                PACK.to_uint(), PACK_START.to_uint(), SA_2_PM, SHIFT1.to_uint(), SHIFT2.to_uint(),
                DST.to_uint(), SRC0.to_uint(), SRC1.to_uint(), SRC2.to_uint(),
                DST_N.to_uint(), SRC0_N.to_uint(), SRC1_N.to_uint(), SRC2_N.to_uint(), MASK_N.to_uint(),
                TILESH.to_uint(), IMM0.to_uint(), IMM1.to_uint());
        DQ->write(instruction);
        wait(clk_period, RESOLUTION);
        wait(0, RESOLUTION);

        MOV_BIT = true;
        SA_BIT = false;
        PM_BIT = false;
        OP_REST = MOP_MOV;
        DST = OPC_VWR_1;
    #if DUAL_BANK_INTERFACE
        SRC0 = OPC_EVENBANK;
    #else
        SRC0 = OPC_DRAM;
    #endif
        WR->write(true);
        AB_mode->write(true);
        pim_mode->write(true);
        row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
        row_aux.range(RF_SEL_BITS - 1, 0) = RF_IB;
        row_addr->write(row_aux);
        col_addr->write(2*i+1);
        instruction = build_instr(MOV_BIT, SA_BIT, PM_BIT, OP_ADD.to_uint(), OP_MASK.to_uint(), OP_REST.to_uint(),
                PACK.to_uint(), PACK_START.to_uint(), SA_2_PM, SHIFT1.to_uint(), SHIFT2.to_uint(),
                DST.to_uint(), SRC0.to_uint(), SRC1.to_uint(), SRC2.to_uint(),
                DST_N.to_uint(), SRC0_N.to_uint(), SRC1_N.to_uint(), SRC2_N.to_uint(), MASK_N.to_uint(),
                TILESH.to_uint(), IMM0.to_uint(), IMM1.to_uint());
        DQ->write(instruction);
        wait(clk_period, RESOLUTION);
        wait(0, RESOLUTION);
    }

    for (i = 0; i < WORDS_PER_VWR; i++) {
        MOV_BIT = false;
        SA_BIT = true;
        PM_BIT = false;
        OP_ADD = ADDOP_ADD;
        PACK = REPACK_12_12;
        PACK_START = 0;
        SA_2_PM = false;
        SHIFT1 = 0;
        DST = OPC_VWR_1;
        DST_N = i;
        SRC0 = OPC_VWR_0;
        SRC0_N = i;
        SRC1 = OPC_VWR_1;
        SRC1_N = i;
        WR->write(true);
        AB_mode->write(true);
        pim_mode->write(true);
        row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
        row_aux.range(RF_SEL_BITS - 1, 0) = RF_IB;
        row_addr->write(row_aux);
        col_addr->write(2*VWR_DRAM_CLK+2*i);
        instruction = build_instr(MOV_BIT, SA_BIT, PM_BIT, OP_ADD.to_uint(), OP_MASK.to_uint(), OP_REST.to_uint(),
                PACK.to_uint(), PACK_START.to_uint(), SA_2_PM, SHIFT1.to_uint(), SHIFT2.to_uint(),
                DST.to_uint(), SRC0.to_uint(), SRC1.to_uint(), SRC2.to_uint(),
                DST_N.to_uint(), SRC0_N.to_uint(), SRC1_N.to_uint(), SRC2_N.to_uint(), MASK_N.to_uint(),
                TILESH.to_uint(), IMM0.to_uint(), IMM1.to_uint());
        DQ->write(instruction);
        wait(clk_period, RESOLUTION);
        wait(0, RESOLUTION);

        MOV_BIT = false;
        SA_BIT = false;
        PM_BIT = false;
        OP_REST = MOP_NOP;
        IMM0 = 0;
        IMM1 = 0;
        WR->write(true);
        AB_mode->write(true);
        pim_mode->write(true);
        row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
        row_aux.range(RF_SEL_BITS - 1, 0) = RF_IB;
        row_addr->write(row_aux);
        col_addr->write(2*VWR_DRAM_CLK+2*i+1);
        instruction = build_instr(MOV_BIT, SA_BIT, PM_BIT, OP_ADD.to_uint(), OP_MASK.to_uint(), OP_REST.to_uint(),
                PACK.to_uint(), PACK_START.to_uint(), SA_2_PM, SHIFT1.to_uint(), SHIFT2.to_uint(),
                DST.to_uint(), SRC0.to_uint(), SRC1.to_uint(), SRC2.to_uint(),
                DST_N.to_uint(), SRC0_N.to_uint(), SRC1_N.to_uint(), SRC2_N.to_uint(), MASK_N.to_uint(),
                TILESH.to_uint(), IMM0.to_uint(), IMM1.to_uint());
        DQ->write(instruction);
        wait(clk_period, RESOLUTION);
        wait(0, RESOLUTION);
    }

    MOV_BIT = true;
    SA_BIT = false;
    PM_BIT = false;
    OP_REST = MOP_MOV;
#if DUAL_BANK_INTERFACE
    DST = OPC_ODD_BANK;
#else
    DST = OPC_DRAM;
#endif
    SRC0 = OPC_VWR_1;
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = RF_IB;
    row_addr->write(row_aux);
    col_addr->write(2*VWR_DRAM_CLK+2*WORDS_PER_VWR);
    instruction = build_instr(MOV_BIT, SA_BIT, PM_BIT, OP_ADD.to_uint(), OP_MASK.to_uint(), OP_REST.to_uint(),
            PACK.to_uint(), PACK_START.to_uint(), SA_2_PM, SHIFT1.to_uint(), SHIFT2.to_uint(),
            DST.to_uint(), SRC0.to_uint(), SRC1.to_uint(), SRC2.to_uint(),
            DST_N.to_uint(), SRC0_N.to_uint(), SRC1_N.to_uint(), SRC2_N.to_uint(), MASK_N.to_uint(),
            TILESH.to_uint(), IMM0.to_uint(), IMM1.to_uint());
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    MOV_BIT = false;
    SA_BIT = false;
    PM_BIT = false;
    OP_REST = MOP_EXIT;
    IMM0 = 0;
    IMM1 = 0;
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = RF_IB;
    row_addr->write(row_aux);
    col_addr->write(2*VWR_DRAM_CLK+2*WORDS_PER_VWR+1);
    instruction = build_instr(MOV_BIT, SA_BIT, PM_BIT, OP_ADD.to_uint(), OP_MASK.to_uint(), OP_REST.to_uint(),
            PACK.to_uint(), PACK_START.to_uint(), SA_2_PM, SHIFT1.to_uint(), SHIFT2.to_uint(),
            DST.to_uint(), SRC0.to_uint(), SRC1.to_uint(), SRC2.to_uint(),
            DST_N.to_uint(), SRC0_N.to_uint(), SRC1_N.to_uint(), SRC2_N.to_uint(), MASK_N.to_uint(),
            TILESH.to_uint(), IMM0.to_uint(), IMM1.to_uint());
    DQ->write(instruction);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    // Write to the HW_LOOP registers
    wait(0, RESOLUTION);
    WR->write(true);
    AB_mode->write(true);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = true;   // Writing to RFs
    row_aux.range(RF_SEL_BITS - 1, 0) = RF_LOOP_REG;    // Filling HW loop registers
    row_addr->write(row_aux);
    col_addr->write(0);

    vwr_tosend = 0; // Prepare VWR to send via DQs
    DQ_tosend = 0;
    DQ_tosend |= (uint64_t) ((2*VWR_DRAM_CLK) & 0xFFFF) << 32;          // Start address
    DQ_tosend |= (uint64_t) ((2*VWR_DRAM_CLK+2*WORDS_PER_VWR-1) & 0xFFFF) << 16;   // End address
    DQ_tosend |= (uint64_t) (10 & 0xFFFF);                                         // Number of iterations
    DQ->write(DQ_tosend);
    wait(clk_period, RESOLUTION);
    wait(0, RESOLUTION);

    // Execute
    for (i = 0; i < VWR_BITS/12; i++) {
        vwr_tosend.range((i+1)*12-1, i*12) = i;
    }
    WR->write(false);
    RD->write(true);
    AB_mode->write(false);
    pim_mode->write(true);
    row_aux.range(ROW_BITS - 1, ROW_BITS - 1) = false;  // Execting the PIM
    row_addr->write(row_aux);
    for (i = 0; i < VWR_DRAM_CLK; i++) {
        col_addr->write(i);
        wait(1 * clk_period, RESOLUTION);
        bank_tosend = vwr_tosend.range((i+1)*DRAM_BITS-1, i*DRAM_BITS);
#if DUAL_BANK_INTERFACE
        even_bus->write(bank_tosend);
        odd_bus->write(allzs);
        wait(1 * clk_period, RESOLUTION);
        odd_bus->write(0);
        even_bus->write(allzs);
#else
        dram_bus->write(bank_tosend);
        wait(1 * clk_period, RESOLUTION);
        dram_bus->write(0);
#endif
    }
    RD->write(false);
    col_addr->write(0);
    wait(1 * clk_period, RESOLUTION);
    RD->write(true);
#if DUAL_BANK_INTERFACE
    odd_bus->write(allzs);
#else
    dram_bus->write(allzs);
#endif
    wait((10*2*WORDS_PER_VWR + 2) * clk_period, RESOLUTION);
    RD->write(false);
    wait(1 * clk_period, RESOLUTION);
    wait(0, RESOLUTION);

#endif  // VWR_DRAM_CLK == 1

    cout << "Simulation finished at time " << sc_time_stamp() << endl;

    // Stop simulation
    sc_stop();

}

