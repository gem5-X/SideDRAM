 /*
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 *
 * Defines for configuring the design.
 *
 */

#ifndef SRC_DEFS_H_
#define SRC_DEFS_H_

#include <cstdlib>
#include <cstdint>
#include "log2.h"

#define MIXED_SIM   0   // 0 if SystemC-only simulation, 1 if mixed SystemC + RTL
#define RECORDING   0   // 1 if recording control signals and VWR inout for detailed layout simulation
#define EN_MODEL    1   // 1 if generating a report for the energy model
#define VCD_TRACE   1   // 1 if generating VCD traces
#define DEBUG       0   // 1 if using assert library and other debug features

#define CLK_PERIOD 3333
#define RESOLUTION SC_PS

// Instruction format
#define BASE_FORMAT             0
#define ENCODED_SHIFT_FORMAT    1

#define INSTR_FORMAT    BASE_FORMAT

// Allowed subword transitions
#define ONE_LEVEL   0
#define TWO_LEVELS  1

#define SW_TRANS    ONE_LEVEL

// Sizing constants
#define WORD_BITS       1536
#define VWR_BITS        8192
#define MASK_BITS       48     // Bit-length of the mask, it's replicated for fitting the word length   // TODO mask is replicated, but shuffling isn't
#define CSD_BITS        48      // Bit-length of the CSD multipliers, considering that 2 bits are needed for each 0/+/-
#define WORDS_PER_VWR   (VWR_BITS / WORD_BITS)  // TODO if this number is larger than 128, like when using 8192-bit VWRs and 48-bit words, micro fields need to be 64-bit
#define MASK_PER_WORD   (WORD_BITS / MASK_BITS)
#define VWR_64B         ((VWR_BITS + 64 - 1) / 64)  // 64-bit groups needed to represent VWR_BITS
#define WORD_64B        ((WORD_BITS + 64 - 1) / 64) // 64-bit groups needed to represent WORD_BITS
#define MASK_64B        ((MASK_BITS + 64 - 1) / 64) // 64-bit groups needed to represent MASK_BITS
#define CSD_64B         ((CSD_BITS + 64 - 1) / 64)  // 64-bit groups needed to represent CSD_BITS

#define CORES_PER_PCH   2
#define DRAM_BITS       8192    // TODO if exploring multi-cycle interfaces, we need to use DRAM_64B and modify accordingly, among other places, in pch_driver
#define VWR_NUM         2   // Number of VWRs per SoftSIMD datapath
#define REG_NUM         4   // Number of registers per SoftSIMD datapath
#define IB_ENTRIES      64
#define CSD_ENTRIES     32
#define SRF_ENTRIES     8
#define MASK_ENTRIES    8
#define SA_CYCLES       0
#define PM_CYCLES       0
#define AAM_ADDR_BITS   3
#define DQ_BITS         64
#define VWR_DQ_CLK      (VWR_BITS / DQ_BITS)
#define VWR_DRAM_CLK    (VWR_BITS / DRAM_BITS)  // If larger than 1, multi-cycle interface between PU and DRAM
#define MC_COL_BITS     2                       // How many column bits are used to index multi-cycle interface betwen VWR and DRAM

// SHIFT constants
#define SA_MAX_SHIFT    7
#define PM_MAX_SHIFT    15

// SoftSIMD constants
#define DUAL_BANK_INTERFACE 0   // Allow for direct, single cycle movement between DRAM bank and VWR
#define HW_LOOP             1   // Hardware loops enable
#define SA_AND_OR           1   // Shift&Add stage includes and and or gates to set the guard bits
#define STATIC_CSD_LEN      1   // Length of CSD operands is set through a memory-mapped register
#define FLEXIBLE_MASK       0   // The DRAM access is controlled by a word-wise bitmask
#define DRAM_SHUFFLE        1   // The tile shuffler can act directly on the DRAM I/O
#define LOCAL_REPACK        0   // The repacker acts within MASK_BITS chunks

// Address mapping constants
#define CHANNEL_BITS    4
#define RANK_BITS       0
#define BG_BITS         2
#define BANK_BITS       2
#define ROW_BITS        14
#define COL_BITS        5
#define GLOBAL_OFFSET   6

#define RF_ADDR_BITS    (LOG2(IB_ENTRIES))  // Combined column and row address bits to address RFs
#define RF_SEL_BITS     (ROW_BITS-1+COL_BITS-RF_ADDR_BITS)  // Number of bits to select the RF

// Define to use or not assert library
#if DEBUG == 0
#define NDEBUG
#endif

#endif /* SRC_DEFS_H_ */
