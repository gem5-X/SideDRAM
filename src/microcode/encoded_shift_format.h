/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Structures defining the format of the ITTs and DLBs for microcode with DLBs of medium size.
 *
 */

#ifndef SRC_MICROCODE_ENCODED_SHIFT_FORMAT_H_
#define SRC_MICROCODE_ENCODED_SHIFT_FORMAT_H_

#include "../opcodes.h"
#include "common_format.h"

#if (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)

// Format of macroinstructions:
// ITT_IDX | IMM/TS_CTRL/INDEX | DST | SRC
// ITT_IDX | SW_LEN | RF_N | DST_N_SA | SRC0_N | DST | SRC0
// ITT_IDX | REPACK | PACK_STA  | DST_N | DST | SRC0_FILL
#define ITT_IDX_BITS    6

#define MACRO_DST_BITS      4
#define MACRO_SRC_BITS      4
#define MACRO_SW_LEN_BITS   3
#define MACRO_RF_N_BITS     (LOG2(CSD_ENTRIES))
#define MACRO_SRC0_N_BITS   (LOG2(WORDS_PER_VWR))
#define MACRO_DST_N_SA_BITS (LOG2(WORDS_PER_VWR))
#define MACRO_REPACK_BITS   5
#define MACRO_PACK_STA_BITS (LOG2(2*WORD_BITS/3))
#define MACRO_DST_N_BITS    (LOG2(WORDS_PER_VWR))
#define TS_STA_BITS         (LOG2(WORDS_PER_VWR))
#define TS_MODE_BITS        2
#define TS_BITS             (TS_STA_BITS+TS_MODE_BITS)
#define MACRO_IMM_BITS      (MAX(MACRO_DST_N_BITS,TS_BITS))

#define MACRO_AUX_BITS      (ITT_IDX_BITS + \
                            (MAX((MACRO_IMM_BITS+MACRO_DST_BITS+MACRO_SRC_BITS),\
                             MAX(MACRO_SW_LEN_BITS+MACRO_RF_N_BITS+MACRO_DST_N_SA_BITS+MACRO_SRC0_N_BITS+MACRO_DST_BITS+MACRO_SRC_BITS,\
                                 MACRO_REPACK_BITS+MACRO_PACK_STA_BITS+MACRO_DST_N_BITS+MACRO_DST_BITS+MACRO_SRC_BITS))))
#define MACRO_BITS          (MACRO_AUX_BITS > 32 ? 64 : 32)
#define SHIFT_ITT_IDX   (MACRO_BITS - ITT_IDX_BITS)

#define MOV_IMM_BITS        MACRO_IMM_BITS
#define MOV_DST_BITS        3  // Also used for Tile Shuffler control and word-masking of DST and SRC in MOV
#define MOV_SRC_BITS        3
#define MOV_DST_SA_BITS     2
#define MOV_DST_N_SA_BITS   MACRO_DST_N_SA_BITS
#define MOV_FIELDS_BITS     (MOV_IMM_BITS+MOV_DST_BITS+MOV_SRC_BITS+MOV_DST_N_SA_BITS+MOV_DST_SA_BITS)
#define SA_SW_LEN_BITS      MACRO_SW_LEN_BITS
#define SA_DST_BITS         3
#define SA_SRC0_BITS        2
#define SA_FIELDS_BITS      (SA_SW_LEN_BITS+SA_DST_BITS+SA_SRC0_BITS)
#define PM_REPACK_BITS      MACRO_REPACK_BITS
#define PM_PACK_STA_BITS    MACRO_PACK_STA_BITS
#define PM_SHIFT_BITS       (LOG2(PM_MAX_SHIFT+1))
#define PM_DST_N_BITS       MACRO_DST_N_BITS
#define PM_FIELDS_BITS      (PM_REPACK_BITS+PM_PACK_STA_BITS+PM_SHIFT_BITS+PM_DST_N_BITS)

// Format of MOV microinstructions: MOV_OP
#define MOV_OP_BITS 4

enum class MOV_DST : uint {
    VWR0 =  0,
    VWR1 =  1,
    VWR2 =  2,
    VWR3 =  3,
    DRAM =  4,
    R0 =    5,
    SA =    6,
};

OPC_STORAGE mov_dst_to_opc (MOV_DST mov_dst);
MOV_DST opc_to_mov_dst (OPC_STORAGE mov_dst);

enum class MOV_SRC : uint {
    VWR0 =  0,
    VWR1 =  1,
    VWR2 =  2,
    VWR3 =  3,
    DRAM =  4,
    SA =    5,
    PM =    6
};

OPC_STORAGE mov_src_to_opc (MOV_SRC mov_src);
MOV_SRC opc_to_mov_src (OPC_STORAGE mov_src);

enum class MOV_DST_SA : uint {
    VWR0 = 0,
    VWR1 = 1,
    VWR2 = 2,
    VWR3 = 3
};

OPC_STORAGE mov_dst_sa_to_opc (MOV_DST_SA mov_dst_sa);
MOV_DST_SA opc_to_mov_dst_sa (OPC_STORAGE mov_dst_sa);

uint16_t build_mov_microinstr (MOP mov_op);

// Format of Shift&Add microinstructions: SA_ADD | SA_SHIFT
#define SA_ADD_BITS     2
#define SA_SHIFT_BITS   2

enum class SA_DST : uint {
    R3 =    0,
    R1 =    1,
    R2 =    2,
    R1R2 =  3,
    VWR =   4,
};

OPC_STORAGE sa_dst_to_opc (SA_DST sa_dst);
SA_DST opc_to_sa_dst (OPC_STORAGE sa_dst);

enum class SA_SRC0 : uint {
    VWR =  0,
    R3 =   1,
    ZERO = 2,
};

OPC_STORAGE sa_src0_to_opc (SA_SRC0 sa_src0);
SA_SRC0 opc_to_sa_src0 (OPC_STORAGE sa_src0);

uint16_t build_sa_microinstr (ADDOP sa_add, uint sa_shift);

// Format of Pack&Mask microinstructions: PM_MASK | PM_DST
#define PM_MASK_BITS    2
#define PM_DST_BITS     1
#define PM_PERM_BITS    1

enum class PM_DST : uint {
    VWR =   0,
    R3 =    1,
};

OPC_STORAGE pm_dst_to_opc (PM_DST pm_dst);
PM_DST opc_to_pm_dst (OPC_STORAGE pm_dst);

uint16_t build_pm_microinstr (MASKOP pm_mask, PM_DST pm_dst, bool pm_perm);

#endif // ENCODED_SHIFT_FORMAT

#endif /* SRC_MICROCODE_ENCODED_SHIFT_FORMAT_H_ */
