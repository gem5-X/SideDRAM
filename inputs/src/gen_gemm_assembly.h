#ifndef __GEN_GEMM_ASSEMBLY_H__
#define __GEN_GEMM_ASSEMBLY_H__

#include <map>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>
#include <math.h>

#include "../../src/defs.h"
#include "../../src/opcodes.h"
#include "../../src/microcode/common_format.h"
#if (INSTR_FORMAT == BASE_FORMAT)
#include "../../src/microcode/base_code.h"
#endif
#if (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)
#include "../../src/microcode/encoded_shift_code.h"
#endif

#define NUM_COL         (1 << (COL_BITS))
#define NUM_ROW         (1 << (ROW_BITS))
#define NUM_BANK        (1 << (BANK_BITS))
#define NUM_BG          (1 << (BG_BITS))
#define SHIFT_CH        GLOBAL_OFFSET  // Shifts to the channel bits
#define SHIFT_COL       (GLOBAL_OFFSET + CHANNEL_BITS)  // Shifts to the column bits
#define SHIFT_RANK      (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS)  // Shifts to the rank bits
#define SHIFT_BG        (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS)   // Shifts to the bg bits
#define SHIFT_BANK      (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS) // Shifts to the bank bits
#define SHIFT_ROW       (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS) // Shifts to the row bits

#if DQ_BITS == 16
typedef uint16_t dq_type;
#elif DQ_BITS == 32
typedef uint32_t dq_type;
#elif DQ_BITS == 64
typedef uint64_t dq_type;
#endif

// Signals if we can reduce an entire multiplication chunk in a tile (UNCONSTRAINED), 
// or if we are constrained by the IB size (IB_CONSTRAINED) or the CSDRF size (CSDRF_CONSTRAINED)
enum class MAPMODE : uint { UNCONSTRAINED, CSDRF_CONSTRAINED, IB_CONSTRAINED };

std::map<MAPMODE, std::string> MAPMODE_STRING = {
    { MAPMODE::UNCONSTRAINED, "UNCONSTRAINED" },
    { MAPMODE::CSDRF_CONSTRAINED, "CSDRF_CONSTRAINED" },
    { MAPMODE::IB_CONSTRAINED, "IB_CONSTRAINED" }
};

uint div_ceil(uint dividend, uint divisor) {
    return (dividend + divisor - 1) / divisor;
}

typedef struct addTreeLayer {
    uint idx;           // Layer index
    uint inputs;        // Number of inputs to the layer
    uint outputs;       // Number of outputs from the layer
    uint additions;     // Number of additions in the layer
    uint ADDOps;        // Number of VFUX ADD operations in the layer
    SWSIZE sw_in;       // Input subword size
    SWSIZE sw_out;      // Output subword size
    SWREPACK sw_change; // Subword repacking operation
    uint PACKOps;     // Number of REPACK operations in the layer
} addTreeLayer;

SWSIZE next_sw(SWSIZE sw) {
    if (sw < SWSIZE::B24 && sw != SWSIZE::INV) {
        return static_cast<SWSIZE>(uint(sw) + 1);
    } else {
        return sw;
    }
}

SWSIZE prev_sw(SWSIZE sw) {
    if (sw > SWSIZE::B3 && sw != SWSIZE::INV) {
        return static_cast<SWSIZE>(uint(sw) - 1);
    } else {
        return sw;
    }
}

SWREPACK repack_sw(SWSIZE in, SWSIZE out) {
    if (in == out) {
        return SWREPACK::INV;
    } else {
        switch (in) {
            case SWSIZE::B3:    return (out == SWSIZE::B4) ? SWREPACK::B3_B4 : SWREPACK::INV;       break;
            case SWSIZE::B4:    return (out == SWSIZE::B6) ? SWREPACK::B4_B6 : SWREPACK::INV;       break;
            case SWSIZE::B6:    return (out == SWSIZE::B8) ? SWREPACK::B6_B8 : SWREPACK::INV;       break;
            case SWSIZE::B8:    return (out == SWSIZE::B12) ? SWREPACK::B8_B12 : SWREPACK::INV;     break;
            case SWSIZE::B12:   return (out == SWSIZE::B16) ? SWREPACK::B12_B16 : SWREPACK::INV;    break;
            case SWSIZE::B16:   return (out == SWSIZE::B24) ? SWREPACK::B16_B24 : SWREPACK::INV;    break;
            default:            return SWREPACK::INV;                                               break;    
        }
    }
}

uint gdc(uint a, uint b) {
    for (;;) {
        if (a == 0) return b;
        b %= a;
        if (b == 0) return a;
        a %= b;
    }
}

uint lcm(uint a, uint b) {
    uint temp = gdc(a, b);
    return temp ? (a / temp * b) : 0;
}

#endif  // __GEN_GEMM_ASSEMBLY_H__