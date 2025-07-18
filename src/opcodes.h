/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Defines for the SoftSIMD opcodes.
 *
 */

#ifndef SRC_OPCODES_H_
#define SRC_OPCODES_H_

#include <map>
#include <stdlib.h>
#include <string>
#include "defs.h"

// CSD digits
#define CSD_ZERO    0
#define CSD_POSONE  1
#define CSD_NEGONE  2

// MOV_OP operation identifiers
enum class MOP : uint {
    EXIT =          0,  // Reset global PC
    NOP =           1,  // Multicycle NOP
    DRAM_RD =       2,  // Read from DRAM to VWR
    DRAM_WR =       3,  // Write to DRAM from VWR
    MOV =           4,  // Move word
    MOV_SAWB =      5,  // Move word and write back SA result to VWR
    TS_FILL =       6,  // Read from VWR to Tile shuffler and shuffle
    TS_RLS =        7,  // Release from Tile shuffler, write to VWR (or DRAM)
#if DRAM_SHUFFLE
    TS_FILL_DRAM =  8,  // Read from DRAM to Tile shuffler and shuffle
#endif
};

const std::map<MOP, std::string> MOP_STRING = {
    { MOP::EXIT,            "MOP_EXIT" },
    { MOP::NOP,             "MOP_NOP" },
    { MOP::DRAM_RD,         "MOP_DRAM_RD" },
    { MOP::DRAM_WR,         "MOP_DRAM_WR" },
    { MOP::MOV,             "MOP_MOV" },
    { MOP::MOV_SAWB,        "MOP_MOV_SAWB" },
    { MOP::TS_FILL,         "MOP_TS_FILL" },
    { MOP::TS_RLS,          "MOP_TS_RLS" },
#if DRAM_SHUFFLE
    { MOP::TS_FILL_DRAM,    "MOP_TS_FILL_DRAM" },
#endif
};

// ADDOP operation identifiers -- (SRC0) >> SHFT +/- SRC1
enum class ADDOP : uint {
    NOP = 0,
    ADD = 1,
    SUB = 2,
    INV = 3,
    MAX = 4,
};

const std::map<ADDOP, std::string> SAOP_STRING = {
    { ADDOP::NOP, "ADDOP_NOP" },
    { ADDOP::ADD, "ADDOP_ADD" },
    { ADDOP::SUB, "ADDOP_SUB" },
    { ADDOP::INV, "ADDOP_INV" },
};

// OPCODES storage identifiers
enum class OPC_STORAGE : uint {
    IB, SRF, MRF,
    R0, R1, R2, R1R2, R3,
    VWR,    // For generic VWR
    VWR0, VWR1,
#if !(DUAL_BANK_INTERFACE)
    VWR2, VWR3, DRAM,
#else
    EVEN_BANK, ODD_BANK,
#endif
    TILESH, ZERO, SA, PM, SAR0
};

const std::map<OPC_STORAGE, std::string> OPC_STORAGE_STRING = {
    { OPC_STORAGE::IB,          "OPC_IB" },
    { OPC_STORAGE::R0,          "OPC_R0" },
    { OPC_STORAGE::R1,          "OPC_R1" },
    { OPC_STORAGE::R2,          "OPC_R2" },
    { OPC_STORAGE::R1R2,        "OPC_R1R2" },
    { OPC_STORAGE::R3,          "OPC_R3" },
    { OPC_STORAGE::SRF,         "OPC_SRF" },
    { OPC_STORAGE::MRF,         "OPC_MRF" },
    { OPC_STORAGE::VWR0,        "OPC_VWR0" },
    { OPC_STORAGE::VWR1,        "OPC_VWR1" },
#if !(DUAL_BANK_INTERFACE)
    { OPC_STORAGE::VWR2,        "OPC_VWR2" },
    { OPC_STORAGE::VWR3,        "OPC_VWR3" },
    { OPC_STORAGE::DRAM,        "OPC_DRAM" },
#else
    { OPC_STORAGE::EVEN_BANK,   "OPC_EVEN_BANK" },
    { OPC_STORAGE::ODD_BANK,    "OPC_ODD_BANK" },
#endif
    { OPC_STORAGE::TILESH,      "OPC_TILESH" },
    { OPC_STORAGE::ZERO,        "OPC_ZERO" },
    { OPC_STORAGE::SA,          "OPC_SA" },
    { OPC_STORAGE::PM,          "OPC_PM" },
    { OPC_STORAGE::SAR0,        "OPC_SAR0" }
};

// MUX select signals
enum class MUX : uint {
    EXT =       0,
    SA =        1,
    PM =        2,
    SRF =       3,
    MRF =       4,
    TILESH =    5,
    ZERO =      6,
    VWR =       7,  // For generic VWR
    VWR0 =      8,
    VWR1 =      9,
#if !(DUAL_BANK_INTERFACE)
    VWR2 =      10,
    VWR3 =      11,
#endif
    R0 =        12,
    R1 =        13,
    R2 =        14,
    R3 =        15,
#if (DUAL_BANK_INTERFACE)
    EVEN_BANK = 16,
    ODD_BANK =  17,
#else
    DRAM =      16,
#endif
};

inline constexpr MUX operator| (MUX lhs, MUX rhs) {
    return static_cast<MUX>(static_cast<uint>(lhs) | static_cast<uint>(rhs));
}

const std::map<MUX, std::string> MUX_STORAGE_STRING = {
    { MUX::EXT,         "MUX_EXT" },
    { MUX::SA,          "MUX_SA" },
    { MUX::PM,          "MUX_PM" },
    { MUX::SRF,         "SRF" },
    { MUX::MRF,         "MUX_MRF" },
    { MUX::TILESH,      "MUX_TILESH" },
    { MUX::ZERO,        "MUX_ZERO" },
    { MUX::VWR0,        "MUX_VWR0" },
    { MUX::VWR1,        "MUX_VWR1" },
#if !(DUAL_BANK_INTERFACE)
    { MUX::VWR2,        "MUX_VWR2" },
    { MUX::VWR3,        "MUX_VWR3" },
#endif
    { MUX::R0,          "MUX_R0" },
    { MUX::R1,          "MUX_R1" },
    { MUX::R2,          "MUX_R2" },
    { MUX::R3,          "MUX_R3" },
#if (DUAL_BANK_INTERFACE)
    { MUX::EVEN_BANK,   "MUX_EVEN_BANK" },
    { MUX::ODD_BANK,    "MUX_ODD_BANK" },
#else
    { MUX::DRAM,        "MUX_DRAM" },
#endif
};

bool is_vwr (MUX mux);

enum class RF_SEL : uint {
    IB =        0,      // Instruction buffer
    SRF =       1,
    MRF =       2,
    CSDRF =     3,
    VWR =       4,
    LOOP_REG =  5,
    CSD_LEN =   6,
};

const std::map<RF_SEL, std::string> RF_SEL_STRING = {
    { RF_SEL::IB,       "RF_IB" },
    { RF_SEL::SRF,      "RF_SRF" },
    { RF_SEL::MRF,      "RF_MRF" },
    { RF_SEL::CSDRF,    "RF_CSDRF" },
    { RF_SEL::VWR,      "RF_VWR" },
    { RF_SEL::LOOP_REG, "RF_LOOP_REG" },
    { RF_SEL::CSD_LEN,  "CSD_LEN" },
};

enum class SWSIZE : uint {
    INV =   0,
    B3 =    1,
    B4 =    2,
    B6 =    3,
    B8 =    4,
    B12 =   5,
    B16 =   6,
    B24 =   7
};

uint swsize_to_uint (SWSIZE swsize);
SWSIZE uint_to_swsize (uint size_uint);

#if SW_TRANS == TWO_LEVELS
enum class SWREPACK : uint {
    INV =       0,
    B3_B3 =     1,
    B4_B4 =     2,
    B6_B6 =     3,
    B8_B8 =     4,
    B12_B12 =   5,
    B16_B16 =   6,
    B24_B24 =   7,
    B3_B4 =     8,
    B3_B6 =     9,
    B4_B6 =     10,
    B4_B8 =     11,
    B6_B8 =     12,
    B6_B12 =    13,
    B8_B12 =    14,
    B8_B16 =    15,
    B12_B16 =   16,
    B12_B24 =   17,
    B16_B24 =   18,
    B24_B16 =   19,
    B24_B12 =   20,
    B16_B12 =   21,
    B16_B8 =    22,
    B12_B8 =    23,
    B12_B6 =    24,
    B8_B6 =     25,
    B8_B4 =     26,
    B6_B4 =     27,
    B6_B3 =     28,
    B4_B3 =     29
};

const std::map<SWREPACK, std::string> REPACK_SEL_STRING = {
    { SWREPACK::INV,        "REPACK_INV" },
    { SWREPACK::B3_B3,      "REPACK_3_3" },
    { SWREPACK::B4_B4,      "REPACK_4_4" },
    { SWREPACK::B6_B6,      "REPACK_6_6" },
    { SWREPACK::B8_B8,      "REPACK_8_8" },
    { SWREPACK::B12_B12,    "REPACK_12_12" },
    { SWREPACK::B16_B16,    "REPACK_16_16" },
    { SWREPACK::B24_B24,    "REPACK_24_24" },
    { SWREPACK::B3_B4,      "REPACK_3_4" },
    { SWREPACK::B3_B6,      "REPACK_3_6" },
    { SWREPACK::B4_B6,      "REPACK_4_6" },
    { SWREPACK::B4_B8,      "REPACK_4_8" },
    { SWREPACK::B6_B8,      "REPACK_6_8" },
    { SWREPACK::B6_B12,     "REPACK_6_12" },
    { SWREPACK::B8_B12,     "REPACK_8_12" },
    { SWREPACK::B8_B16,     "REPACK_8_16" },
    { SWREPACK::B12_B16,    "REPACK_12_16" },
    { SWREPACK::B12_B24,    "REPACK_12_24" },
    { SWREPACK::B16_B24,    "REPACK_16_24" },
    { SWREPACK::B24_B16,    "REPACK_24_16" },
    { SWREPACK::B24_B12,    "REPACK_24_12" },
    { SWREPACK::B16_B12,    "REPACK_16_12" },
    { SWREPACK::B16_B8,     "REPACK_16_8" },
    { SWREPACK::B12_B8,     "REPACK_12_8" },
    { SWREPACK::B12_B6,     "REPACK_12_6" },
    { SWREPACK::B8_B6,      "REPACK_8_6" },
    { SWREPACK::B8_B4,      "REPACK_8_4" },
    { SWREPACK::B6_B4,      "REPACK_6_4" },
    { SWREPACK::B6_B3,      "REPACK_6_3" },
    { SWREPACK::B4_B3,      "REPACK_4_3" },
};
#endif

#if SW_TRANS == ONE_LEVEL
enum class SWREPACK : uint {    // Pengbo version
    INV =       31,
    B3_B3 =     12,
    B4_B4 =     13,
    B6_B6 =     14,
    B8_B8 =     15,
    B12_B12 =   28,
    B16_B16 =   29,
    B24_B24 =   30,
    B3_B4 =     0,
    B4_B6 =     2,
    B6_B8 =     4,
    B8_B12 =    6,
    B12_B16 =   8,
    B16_B24 =   10,
    B24_B16 =   11,
    B16_B12 =   9,
    B12_B8 =    7,
    B8_B6 =     5,
    B6_B4 =     3,
    B4_B3 =     1,
};

// Constant list of the SWREPACK values to iterate over
const SWREPACK SWREPACK_LIST[] = {
    SWREPACK::B3_B3,
    SWREPACK::B4_B4,
    SWREPACK::B6_B6,
    SWREPACK::B8_B8,
    SWREPACK::B12_B12,
    SWREPACK::B16_B16,
    SWREPACK::B24_B24,
    SWREPACK::B3_B4,
    SWREPACK::B4_B6,
    SWREPACK::B6_B8,
    SWREPACK::B8_B12,
    SWREPACK::B12_B16,
    SWREPACK::B16_B24,
    SWREPACK::B24_B16,
    SWREPACK::B16_B12,
    SWREPACK::B12_B8,
    SWREPACK::B8_B6,
    SWREPACK::B6_B4,
    SWREPACK::B4_B3,
};

const std::map<SWREPACK, std::string> REPACK_SEL_STRING = {
    { SWREPACK::INV,        "REPACK_INV" },
    { SWREPACK::B3_B3,      "REPACK_3_3" },
    { SWREPACK::B4_B4,      "REPACK_4_4" },
    { SWREPACK::B6_B6,      "REPACK_6_6" },
    { SWREPACK::B8_B8,      "REPACK_8_8" },
    { SWREPACK::B12_B12,    "REPACK_12_12" },
    { SWREPACK::B16_B16,    "REPACK_16_16" },
    { SWREPACK::B24_B24,    "REPACK_24_24" },
    { SWREPACK::B3_B4,      "REPACK_3_4" },
    { SWREPACK::B4_B6,      "REPACK_4_6" },
    { SWREPACK::B6_B8,      "REPACK_6_8" },
    { SWREPACK::B8_B12,     "REPACK_8_12" },
    { SWREPACK::B12_B16,    "REPACK_12_16" },
    { SWREPACK::B16_B24,    "REPACK_16_24" },
    { SWREPACK::B24_B16,    "REPACK_24_16" },
    { SWREPACK::B16_B12,    "REPACK_16_12" },
    { SWREPACK::B12_B8,     "REPACK_12_8" },
    { SWREPACK::B8_B6,      "REPACK_8_6" },
    { SWREPACK::B6_B4,      "REPACK_6_4" },
    { SWREPACK::B4_B3,      "REPACK_4_3" },
};
#endif

SWSIZE repack_to_insize (SWREPACK swrepack);
SWSIZE repack_to_outsize (SWREPACK swrepack);

uint repack_to_shift (SWREPACK repack);

enum class MASKOP : uint {
    NOP = 0,
    AND = 1,
    OR =  2,
    XOR = 3
};

const std::map<MASKOP, std::string> MASK_SEL_STRING = {
    { MASKOP::NOP, "MASK_NOP" },
    { MASKOP::AND, "MASK_AND" },
    { MASKOP::OR,  "MASK_OR" },
    { MASKOP::XOR, "MASK_XOR" },
};

enum class TS_MODE : uint {
    NOP =       0,
    SHIFT =     1,
    BROADCAST = 2,
    REPEAT =    3
};

const std::map<TS_MODE, std::string> TS_MODE_STRING = {
    { TS_MODE::NOP,         "TS_MODE_NOP" },
    { TS_MODE::SHIFT,       "TS_MODE_SHIFT" },
    { TS_MODE::BROADCAST,   "TS_MODE_BROADCAST" },
    { TS_MODE::REPEAT,      "TS_MODE_REPEAT" },
};

#endif /* SRC_OPCODES_H_ */
