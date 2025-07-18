/* 
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 * 
 * Basic include for the CnM design.
 *
 */

#ifndef CNM_BASE_H
#define CNM_BASE_H

#include <map>
#include <string>
#include "systemc.h"
#include "defs.h"
#include "sc_functions.h"

// RoBaBgRaCoCh mapping
#define CH_END          GLOBAL_OFFSET
#define CH_STA          GLOBAL_OFFSET + CHANNEL_BITS - 1
#define CO_END          GLOBAL_OFFSET + CHANNEL_BITS
#define CO_STA          GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS - 1
#define RA_END          GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS
#define RA_STA          GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS - 1
#define BG_END          GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS
#define BG_STA          GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS - 1
#define BA_END          GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS
#define BA_STA          GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS - 1
#define RO_END          GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS
#define RO_STA          GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS + ROW_BITS - 1
#define ADDR_TOTAL_BITS GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS + ROW_BITS

#include "opcodes.h"
#include "microcode/common_format.h"
#if (INSTR_FORMAT == BASE_FORMAT)
#include "microcode/base_code.h"
#endif

#if (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)
#include "microcode/encoded_shift_code.h"
#endif

#if DQ_BITS == 16
typedef uint16_t dq_type;
#elif DQ_BITS == 32
typedef uint32_t dq_type;
#elif DQ_BITS == 64
typedef uint64_t dq_type;
#endif


#endif
