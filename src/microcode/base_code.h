/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Structures defining the contents of the ITTs and DLBs for microcode with DLBs of small size.
 *
 */

#ifndef SRC_MICROCODE_BASE_CODE_H_
#define SRC_MICROCODE_BASE_CODE_H_

#include "base_format.h"
#include "common_format.h"

#if (INSTR_FORMAT == BASE_FORMAT)

// Contents of the MOV DLB
enum class DLB_MOV_IDX : uint {
    EXIT, NOP, DRAM_RD, DRAM_WR, MOV, MOV_SAWB, TS_FILL, TS_RLS,
#if DRAM_SHUFFLE
    TS_FILL_DRAM,
#endif
    MAX,
};

const uint16_t dlb_mov_contents[uint(DLB_MOV_IDX::MAX)] = {
    build_mov_microinstr(MOP::EXIT),
    build_mov_microinstr(MOP::NOP),
    build_mov_microinstr(MOP::DRAM_RD),
    build_mov_microinstr(MOP::DRAM_WR),
    build_mov_microinstr(MOP::MOV),
    build_mov_microinstr(MOP::MOV_SAWB),
    build_mov_microinstr(MOP::TS_FILL),
    build_mov_microinstr(MOP::TS_RLS),
#if DRAM_SHUFFLE
    build_mov_microinstr(MOP::TS_FILL_DRAM),
#endif
};

// Contents of the S&A DLB
enum class DLB_SA_IDX : uint {
    NOP, ADD, SUB, INV, MAX
};

const uint16_t dlb_sa_contents[uint(DLB_SA_IDX::MAX)] = {
    build_sa_microinstr(ADDOP::NOP),
    build_sa_microinstr(ADDOP::ADD),
    build_sa_microinstr(ADDOP::SUB),
    build_sa_microinstr(ADDOP::INV)
};

// Contents of the P&M DLB
enum class DLB_PM_IDX : uint {
    NOP_TOVWR, NOP_TOR3, NOP_TOVWR_PERM, MAX
};

const uint16_t dlb_pm_contents[uint(DLB_PM_IDX::MAX)] = {
    build_pm_microinstr(MASKOP::NOP, PM_DST::VWR, false),
    build_pm_microinstr(MASKOP::NOP, PM_DST::R3, false),
    build_pm_microinstr(MASKOP::NOP, PM_DST::VWR, true)
};

// ITT indeces
enum class MACRO_IDX : uint {
    SAFE_STATE =            0,
    EXIT =                  1,
    NOP =                   2,
    RLB =                   3,
    WLB =                   5,
    VMV =                   6,
    RMV =                   7,
    GLMV =                  8,
#if DRAM_SHUFFLE    
    GLMV_FROMDRAM =         10,
    PACK_TOVWR =            13,
    PACK_TOR3 =             14,
    PERM_TOVWR =            15,
    PERM_TOR3 =             16,
    PERM_TOBOTH =           17,
    VFUX_SHIFT_FROMVWR =    19,
    VFUX_SHIFT_FROMR3 =     20,
    VFUX_SHIFT_FROMTOVWR =  21,
    VFUX_ADD_FROMVWR =      22,
    VFUX_ADD_FROMR3 =       23,
    VFUX_ADD_FROMTOVWR =    24,
    VFUX_SUB_FROMVWR =      25,
    VFUX_SUB_FROMR3 =       26,
    VFUX_SUB_FROMTOVWR =    27, 
    VFUX_MUL =              28,
    MAX =                   29
#else   
    PACK_TOVWR =            10,
    PACK_TOR3 =             11,
    PERM_TOVWR =            12,
    PERM_TOR3 =             13,
    PERM_TOBOTH =           14,
    VFUX_SHIFT_FROMVWR =    16,
    VFUX_SHIFT_FROMR3 =     17,
    VFUX_SHIFT_FROMTOVWR =  18,
    VFUX_ADD_FROMVWR =      19,
    VFUX_ADD_FROMR3 =       20,
    VFUX_ADD_FROMTOVWR =    21,
    VFUX_SUB_FROMVWR =      22,
    VFUX_SUB_FROMR3 =       23,
    VFUX_SUB_FROMTOVWR =    24,
    VFUX_MUL =              25,
    MAX =                   26
#endif
};

// Contents of the MOV ITT
const itt_field itt_mov_contents[uint(MACRO_IDX::MAX)] = {
    { false,    0,                                  true },     // SAFE_STATE
    { true,     uint(DLB_MOV_IDX::EXIT),            true },     // EXIT
    { true,     uint(DLB_MOV_IDX::NOP),             true },     // NOP
    { false,    0,                                  false },    // RLB  // Need one idle cycle to wait for bank contents
    { true,     uint(DLB_MOV_IDX::DRAM_RD),         true },     //
    { true,     uint(DLB_MOV_IDX::DRAM_WR),         true },     // WLB
    { true,     uint(DLB_MOV_IDX::MOV),             true },     // VMV
    { true,     uint(DLB_MOV_IDX::MOV_SAWB),        true },     // RMV
    { true,     uint(DLB_MOV_IDX::TS_FILL),         false },    // GLMV
    { true,     uint(DLB_MOV_IDX::TS_RLS),          true },     //
#if DRAM_SHUFFLE
    { false,    0,                                  false },    // GLMV_FROMDRAM    // Need one idle cycle to wait for bank contents
    { true,     uint(DLB_MOV_IDX::TS_FILL_DRAM),    false },    //
    { true,     uint(DLB_MOV_IDX::TS_RLS),          true },     //
#endif
    { true,     uint(DLB_MOV_IDX::MOV),             true },     // PACK_TOVWR
    { false,    0,                                  true },     // PACK_TOR3
    { true,     uint(DLB_MOV_IDX::MOV),             true },     // PERM_TOVWR
    { false,    0,                                  true },     // PERM_TOR3
    { false,    0,                                  false },    // PERM_TOBOTH
    { true,     uint(DLB_MOV_IDX::MOV),             true },     //
    { true,     uint(DLB_MOV_IDX::MOV),             true },     // VFUX_SHIFT_FROMVWR
    { false,    0,                                  true },     // VFUX_SHIFT_FROMR3
    { true,     uint(DLB_MOV_IDX::MOV_SAWB),        true },     // VFUX_SHIFT_FROMTOVWR
    { true,     uint(DLB_MOV_IDX::MOV),             true },     // VFUX_ADD_FROMVWR
    { false,    0,                                  true },     // VFUX_ADD_FROMR3
    { true,     uint(DLB_MOV_IDX::MOV_SAWB),        true },     // VFUX_ADD_FROMTOVWR
    { true,     uint(DLB_MOV_IDX::MOV),             true },     // VFUX_SUB_FROMVWR
    { false,    0,                                  true },     // VFUX_SUB_FROMR3
    { true,     uint(DLB_MOV_IDX::MOV_SAWB),        true },     // VFUX_SUB_FROMTOVWR
    { false,    0,                                  true },     // VFUX_MUL
};

// Contents of the S&A ITT
const itt_field itt_sa_contents[uint(MACRO_IDX::MAX)] = {
    { false,    0,                      true },     // SAFE_STATE
    { false,    0,                      true },     // EXIT
    { false,    0,                      true },     // NOP
    { false,    0,                      false },    // RLB  // Need one idle cycle to wait for bank contents
    { false,    0,                      true },     //
    { false,    0,                      true },     // WLB
    { false,    0,                      true },     // VMV
    { true,     uint(DLB_SA_IDX::NOP),  true },     // RMV
    { false,    0,                      false },    // GLMV
    { false,    0,                      true },     //
#if DRAM_SHUFFLE
    { false,    0,                      false },    // GLMV_FROMDRAM
    { false,    0,                      false },    //
    { false,    0,                      true },     //
#endif
    { false,    0,                      true },     // PACK_TOVWR
    { false,    0,                      true },     // PACK_TOR3
    { false,    0,                      true },     // PERM_TOVWR
    { false,    0,                      true },     // PERM_TOR3
    { false,    0,                      false },    // PERM_TOBOTH
    { false,    0,                      true },     //
    { true,     uint(DLB_SA_IDX::NOP),  true },     // VFUX_SHIFT_FROMVWR
    { true,     uint(DLB_SA_IDX::NOP),  true },     // VFUX_SHIFT_FROMR3
    { true,     uint(DLB_SA_IDX::NOP),  true },     // VFUX_SHIFT_FROMTOVWR
    { true,     uint(DLB_SA_IDX::ADD),  true },     // VFUX_ADD_FROMVWR
    { true,     uint(DLB_SA_IDX::ADD),  true },     // VFUX_ADD_FROMR3
    { true,     uint(DLB_SA_IDX::ADD),  true },     // VFUX_ADD_FROMTOVWR
    { true,     uint(DLB_SA_IDX::SUB),  true },     // VFUX_SUB_FROMVWR
    { true,     uint(DLB_SA_IDX::SUB),  true },     // VFUX_SUB_FROMR3
    { true,     uint(DLB_SA_IDX::SUB),  true },     // VFUX_SUB_FROMTOVWR
    { false,    0,                      true },     // VFUX_MUL
};

// Contents of the P&M ITT
const itt_field itt_pm_contents[uint(MACRO_IDX::MAX)] = {
    { false,    0,                                  true },     // SAFE_STATE
    { false,    0,                                  true },     // EXIT
    { false,    0,                                  true },     // NOP
    { false,    0,                                  false },    // RLB  // Need one idle cycle to wait for bank contents
    { false,    0,                                  true },     //
    { false,    0,                                  true },     // WLB
    { false,    0,                                  true },     // VMV
    { false,    0,                                  false },    // RMV
    { false,    0,                                  false },    // GLMV
    { false,    0,                                  true },     //
#if DRAM_SHUFFLE
    { false,    0,                                  false },    // GLMV_FROMDRAM
    { false,    0,                                  false },    //
    { false,    0,                                  true },     //
#endif
    { true,     uint(DLB_PM_IDX::NOP_TOVWR),        true },     // PACK_TOVWR
    { true,     uint(DLB_PM_IDX::NOP_TOR3),         true },     // PACK_TOR3
    { true,     uint(DLB_PM_IDX::NOP_TOVWR_PERM),   true },     // PERM_TOVWR
    { true,     uint(DLB_PM_IDX::NOP_TOR3),         true },     // PERM_TOR3
    { true,     uint(DLB_PM_IDX::NOP_TOR3),         false },    // PERM_TOBOTH
    { true,     uint(DLB_PM_IDX::NOP_TOVWR_PERM),   true },     //
    { false,    0,                                  true },     // VFUX_SHIFT_FROMVWR
    { false,    0,                                  true },     // VFUX_SHIFT_FROMR3
    { false,    0,                                  true },     // VFUX_ADD_FROMVWR
    { false,    0,                                  true },     // VFUX_ADD_FROMR3
    { false,    0,                                  true },     // VFUX_ADD_FROMTOVWR
    { false,    0,                                  true },     // VFUX_ADD_FROMTOVWR
    { false,    0,                                  true },     // VFUX_SUB_FROMVWR
    { false,    0,                                  true },     // VFUX_SUB_FROMR3
    { false,    0,                                  true },     // VFUX_SUB_FROMTOVWR
    { false,    0,                                  true },     // VFUX_MUL
};

// Translation from multiplication sequencer info to the S&A microinstruction index
const DLB_SA_IDX mult_seq_microinstr[uint(ADDOP::MAX)] = {
    DLB_SA_IDX::NOP, DLB_SA_IDX::ADD, DLB_SA_IDX::SUB, DLB_SA_IDX::INV
};

#endif  // SSIZE

#endif /* SRC_MICROCODE_BASE_CODE_H_ */
